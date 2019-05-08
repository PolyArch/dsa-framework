/* Ada Ravenscar thread support.

   Copyright (C) 2004-2018 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "ada-lang.h"
#include "target.h"
#include "inferior.h"
#include "command.h"
#include "ravenscar-thread.h"
#include "observer.h"
#include "gdbcmd.h"
#include "top.h"
#include "regcache.h"
#include "objfiles.h"

/* This module provides support for "Ravenscar" tasks (Ada) when
   debugging on bare-metal targets.

   The typical situation is when debugging a bare-metal target over
   the remote protocol. In that situation, the system does not know
   about high-level comcepts such as threads, only about some code
   running on one or more CPUs. And since the remote protocol does not
   provide any handling for CPUs, the de facto standard for handling
   them is to have one thread per CPU, where the thread's ptid has
   its lwp field set to the CPU number (eg: 1 for the first CPU,
   2 for the second one, etc).  This module will make that assumption.

   This module then creates and maintains the list of threads based
   on the list of Ada tasks, with one thread per Ada tasks. The convention
   is that threads corresponding to the CPUs (see assumption above)
   have a ptid_t of the form (PID, LWP, 0), which threads corresponding
   to our Ada tasks have a ptid_t of the form (PID, 0, TID) where TID
   is the Ada task's ID as extracted from Ada runtime information.

   Switching to a given Ada tasks (or its underlying thread) is performed
   by fetching the registers of that tasks from the memory area where
   the registers were saved.  For any of the other operations, the
   operation is performed by first finding the CPU on which the task
   is running, switching to its corresponding ptid, and then performing
   the operation on that ptid using the target beneath us.  */

/* If non-null, ravenscar task support is enabled.  */
static int ravenscar_task_support = 1;

/* This module's target-specific operations.  */
static struct target_ops ravenscar_ops;

/* PTID of the last thread that received an event.
   This can be useful to determine the associated task that received
   the event, to make it the current task.  */
static ptid_t base_ptid;

static const char running_thread_name[] = "__gnat_running_thread_table";

static const char known_tasks_name[] = "system__tasking__debug__known_tasks";
static const char first_task_name[] = "system__tasking__debug__first_task";

static const char ravenscar_runtime_initializer[] =
  "system__bb__threads__initialize";

static void ravenscar_update_thread_list (struct target_ops *ops);
static ptid_t ravenscar_active_task (int cpu);
static const char *ravenscar_extra_thread_info (struct target_ops *self,
						struct thread_info *tp);
static int ravenscar_thread_alive (struct target_ops *ops, ptid_t ptid);
static void ravenscar_fetch_registers (struct target_ops *ops,
                                       struct regcache *regcache, int regnum);
static void ravenscar_store_registers (struct target_ops *ops,
                                       struct regcache *regcache, int regnum);
static void ravenscar_prepare_to_store (struct target_ops *self,
					struct regcache *regcache);
static void ravenscar_resume (struct target_ops *ops, ptid_t ptid, int step,
			      enum gdb_signal siggnal);
static void ravenscar_mourn_inferior (struct target_ops *ops);
static void ravenscar_update_inferior_ptid (void);
static int has_ravenscar_runtime (void);
static int ravenscar_runtime_initialized (void);
static void ravenscar_inferior_created (struct target_ops *target,
					int from_tty);

/* Return nonzero iff PTID corresponds to a ravenscar task.  */

static int
is_ravenscar_task (ptid_t ptid)
{
  /* By construction, ravenscar tasks have their LWP set to zero.
     Also make sure that the TID is nonzero, as some remotes, when
     asked for the list of threads, will return the first thread
     as having its TID set to zero.  For instance, TSIM version
     2.0.48 for LEON3 sends 'm0' as a reply to the 'qfThreadInfo'
     query, which the remote protocol layer then treats as a thread
     whose TID is 0.  This is obviously not a ravenscar task.  */
  return ptid_get_lwp (ptid) == 0 && ptid_get_tid (ptid) != 0;
}

/* Given PTID, which can be either a ravenscar task or a CPU thread,
   return which CPU that ptid is running on.

   This assume that PTID is a valid ptid_t.  Otherwise, a gdb_assert
   will be triggered.  */

static int
ravenscar_get_thread_base_cpu (ptid_t ptid)
{
  int base_cpu;

  if (is_ravenscar_task (ptid))
    {
      struct ada_task_info *task_info = ada_get_task_info_from_ptid (ptid);

      gdb_assert (task_info != NULL);
      base_cpu = task_info->base_cpu;
    }
  else
    {
      /* We assume that the LWP of the PTID is equal to the CPU number.  */
      base_cpu = ptid_get_lwp (ptid);
    }

  return base_cpu;
}

/* Given a ravenscar task (identified by its ptid_t PTID), return nonzero
   if this task is the currently active task on the cpu that task is
   running on.

   In other words, this function determine which CPU this task is
   currently running on, and then return nonzero if the CPU in question
   is executing the code for that task.  If that's the case, then
   that task's registers are in the CPU bank.  Otherwise, the task
   is currently suspended, and its registers have been saved in memory.  */

static int
ravenscar_task_is_currently_active (ptid_t ptid)
{
  ptid_t active_task_ptid
    = ravenscar_active_task (ravenscar_get_thread_base_cpu (ptid));

  return ptid_equal (ptid, active_task_ptid);
}

/* Return the CPU thread (as a ptid_t) on which the given ravenscar
   task is running.

   This is the thread that corresponds to the CPU on which the task
   is running.  */

static ptid_t
get_base_thread_from_ravenscar_task (ptid_t ptid)
{
  int base_cpu;

  if (!is_ravenscar_task (ptid))
    return ptid;

  base_cpu = ravenscar_get_thread_base_cpu (ptid);
  return ptid_build (ptid_get_pid (ptid), base_cpu, 0);
}

/* Fetch the ravenscar running thread from target memory and
   update inferior_ptid accordingly.  */

static void
ravenscar_update_inferior_ptid (void)
{
  int base_cpu;

  base_ptid = inferior_ptid;

  gdb_assert (!is_ravenscar_task (inferior_ptid));
  base_cpu = ravenscar_get_thread_base_cpu (base_ptid);

  /* If the runtime has not been initialized yet, the inferior_ptid is
     the only ptid that there is.  */
  if (!ravenscar_runtime_initialized ())
    return;

  /* Make sure we set base_ptid before calling ravenscar_active_task
     as the latter relies on it.  */
  inferior_ptid = ravenscar_active_task (base_cpu);
  gdb_assert (!ptid_equal (inferior_ptid, null_ptid));

  /* The running thread may not have been added to
     system.tasking.debug's list yet; so ravenscar_update_thread_list
     may not always add it to the thread list.  Add it here.  */
  if (!find_thread_ptid (inferior_ptid))
    add_thread (inferior_ptid);
}

/* The Ravenscar Runtime exports a symbol which contains the ID of
   the thread that is currently running.  Try to locate that symbol
   and return its associated minimal symbol.
   Return NULL if not found.  */

static struct bound_minimal_symbol
get_running_thread_msymbol (void)
{
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol (running_thread_name, NULL, NULL);
  if (!msym.minsym)
    /* Older versions of the GNAT runtime were using a different
       (less ideal) name for the symbol where the active thread ID
       is stored.  If we couldn't find the symbol using the latest
       name, then try the old one.  */
    msym = lookup_minimal_symbol ("running_thread", NULL, NULL);

  return msym;
}

/* Return True if the Ada Ravenscar run-time can be found in the
   application.  */

static int
has_ravenscar_runtime (void)
{
  struct bound_minimal_symbol msym_ravenscar_runtime_initializer =
    lookup_minimal_symbol (ravenscar_runtime_initializer, NULL, NULL);
  struct bound_minimal_symbol msym_known_tasks =
    lookup_minimal_symbol (known_tasks_name, NULL, NULL);
  struct bound_minimal_symbol msym_first_task =
    lookup_minimal_symbol (first_task_name, NULL, NULL);
  struct bound_minimal_symbol msym_running_thread
    = get_running_thread_msymbol ();

  return (msym_ravenscar_runtime_initializer.minsym
	  && (msym_known_tasks.minsym || msym_first_task.minsym)
	  && msym_running_thread.minsym);
}

/* Return True if the Ada Ravenscar run-time can be found in the
   application, and if it has been initialized on target.  */

static int
ravenscar_runtime_initialized (void)
{
  return (!(ptid_equal (ravenscar_active_task (1), null_ptid)));
}

/* Return the ID of the thread that is currently running.
   Return 0 if the ID could not be determined.  */

static CORE_ADDR
get_running_thread_id (int cpu)
{
  struct bound_minimal_symbol object_msym = get_running_thread_msymbol ();
  int object_size;
  int buf_size;
  gdb_byte *buf;
  CORE_ADDR object_addr;
  struct type *builtin_type_void_data_ptr =
    builtin_type (target_gdbarch ())->builtin_data_ptr;

  if (!object_msym.minsym)
    return 0;

  object_size = TYPE_LENGTH (builtin_type_void_data_ptr);
  object_addr = (BMSYMBOL_VALUE_ADDRESS (object_msym)
		 + (cpu - 1) * object_size);
  buf_size = object_size;
  buf = (gdb_byte *) alloca (buf_size);
  read_memory (object_addr, buf, buf_size);
  return extract_typed_address (buf, builtin_type_void_data_ptr);
}

static void
ravenscar_resume (struct target_ops *ops, ptid_t ptid, int step,
		  enum gdb_signal siggnal)
{
  struct target_ops *beneath = find_target_beneath (ops);

  inferior_ptid = base_ptid;
  beneath->to_resume (beneath, base_ptid, step, siggnal);
}

static ptid_t
ravenscar_wait (struct target_ops *ops, ptid_t ptid,
                struct target_waitstatus *status,
                int options)
{
  struct target_ops *beneath = find_target_beneath (ops);
  ptid_t event_ptid;

  inferior_ptid = base_ptid;
  event_ptid = beneath->to_wait (beneath, base_ptid, status, 0);
  /* Find any new threads that might have been created, and update
     inferior_ptid to the active thread.

     Only do it if the program is still alive, though.  Otherwise,
     this causes problems when debugging through the remote protocol,
     because we might try switching threads (and thus sending packets)
     after the remote has disconnected.  */
  if (status->kind != TARGET_WAITKIND_EXITED
      && status->kind != TARGET_WAITKIND_SIGNALLED)
    {
      inferior_ptid = event_ptid;
      ravenscar_update_thread_list (ops);
      ravenscar_update_inferior_ptid ();
    }
  return inferior_ptid;
}

/* Add the thread associated to the given TASK to the thread list
   (if the thread has already been added, this is a no-op).  */

static void
ravenscar_add_thread (struct ada_task_info *task)
{
  if (find_thread_ptid (task->ptid) == NULL)
    add_thread (task->ptid);
}

static void
ravenscar_update_thread_list (struct target_ops *ops)
{
  ada_build_task_list ();

  /* Do not clear the thread list before adding the Ada task, to keep
     the thread that the process stratum has included into it
     (base_ptid) and the running thread, that may not have been included
     to system.tasking.debug's list yet.  */

  iterate_over_live_ada_tasks (ravenscar_add_thread);
}

static ptid_t
ravenscar_active_task (int cpu)
{
  CORE_ADDR tid = get_running_thread_id (cpu);

  if (tid == 0)
    return null_ptid;
  else
    return ptid_build (ptid_get_pid (base_ptid), 0, tid);
}

static const char *
ravenscar_extra_thread_info (struct target_ops *self, struct thread_info *tp)
{
  return "Ravenscar task";
}

static int
ravenscar_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  /* Ravenscar tasks are non-terminating.  */
  return 1;
}

static const char *
ravenscar_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char buf[30];

  snprintf (buf, sizeof (buf), "Thread %#x", (int) ptid_get_tid (ptid));
  return buf;
}

static void
ravenscar_fetch_registers (struct target_ops *ops,
                           struct regcache *regcache, int regnum)
{
  struct target_ops *beneath = find_target_beneath (ops);
  ptid_t ptid = regcache_get_ptid (regcache);

  if (ravenscar_runtime_initialized ()
      && is_ravenscar_task (ptid)
      && !ravenscar_task_is_currently_active (ptid))
    {
      struct gdbarch *gdbarch = regcache->arch ();
      struct ravenscar_arch_ops *arch_ops
	= gdbarch_ravenscar_ops (gdbarch);

      arch_ops->to_fetch_registers (regcache, regnum);
    }
  else
    beneath->to_fetch_registers (beneath, regcache, regnum);
}

static void
ravenscar_store_registers (struct target_ops *ops,
                           struct regcache *regcache, int regnum)
{
  struct target_ops *beneath = find_target_beneath (ops);
  ptid_t ptid = regcache_get_ptid (regcache);

  if (ravenscar_runtime_initialized ()
      && is_ravenscar_task (ptid)
      && !ravenscar_task_is_currently_active (ptid))
    {
      struct gdbarch *gdbarch = regcache->arch ();
      struct ravenscar_arch_ops *arch_ops
	= gdbarch_ravenscar_ops (gdbarch);

      arch_ops->to_store_registers (regcache, regnum);
    }
  else
    beneath->to_store_registers (beneath, regcache, regnum);
}

static void
ravenscar_prepare_to_store (struct target_ops *self,
			    struct regcache *regcache)
{
  struct target_ops *beneath = find_target_beneath (self);
  ptid_t ptid = regcache_get_ptid (regcache);

  if (ravenscar_runtime_initialized ()
      && is_ravenscar_task (ptid)
      && !ravenscar_task_is_currently_active (ptid))
    {
      struct gdbarch *gdbarch = regcache->arch ();
      struct ravenscar_arch_ops *arch_ops
	= gdbarch_ravenscar_ops (gdbarch);

      arch_ops->to_prepare_to_store (regcache);
    }
  else
    beneath->to_prepare_to_store (beneath, regcache);
}

/* Implement the to_stopped_by_sw_breakpoint target_ops "method".  */

static int
ravenscar_stopped_by_sw_breakpoint (struct target_ops *ops)
{
  ptid_t saved_ptid = inferior_ptid;
  struct target_ops *beneath = find_target_beneath (ops);
  int result;

  inferior_ptid = get_base_thread_from_ravenscar_task (saved_ptid);
  result = beneath->to_stopped_by_sw_breakpoint (beneath);
  inferior_ptid = saved_ptid;
  return result;
}

/* Implement the to_stopped_by_hw_breakpoint target_ops "method".  */

static int
ravenscar_stopped_by_hw_breakpoint (struct target_ops *ops)
{
  ptid_t saved_ptid = inferior_ptid;
  struct target_ops *beneath = find_target_beneath (ops);
  int result;

  inferior_ptid = get_base_thread_from_ravenscar_task (saved_ptid);
  result = beneath->to_stopped_by_hw_breakpoint (beneath);
  inferior_ptid = saved_ptid;
  return result;
}

/* Implement the to_stopped_by_watchpoint target_ops "method".  */

static int
ravenscar_stopped_by_watchpoint (struct target_ops *ops)
{
  ptid_t saved_ptid = inferior_ptid;
  struct target_ops *beneath = find_target_beneath (ops);
  int result;

  inferior_ptid = get_base_thread_from_ravenscar_task (saved_ptid);
  result = beneath->to_stopped_by_watchpoint (beneath);
  inferior_ptid = saved_ptid;
  return result;
}

/* Implement the to_stopped_data_address target_ops "method".  */

static int
ravenscar_stopped_data_address (struct target_ops *ops, CORE_ADDR *addr_p)
{
  ptid_t saved_ptid = inferior_ptid;
  struct target_ops *beneath = find_target_beneath (ops);
  int result;

  inferior_ptid = get_base_thread_from_ravenscar_task (saved_ptid);
  result = beneath->to_stopped_data_address (beneath, addr_p);
  inferior_ptid = saved_ptid;
  return result;
}

static void
ravenscar_mourn_inferior (struct target_ops *ops)
{
  struct target_ops *beneath = find_target_beneath (ops);

  base_ptid = null_ptid;
  beneath->to_mourn_inferior (beneath);
  unpush_target (&ravenscar_ops);
}

/* Implement the to_core_of_thread target_ops "method".  */

static int
ravenscar_core_of_thread (struct target_ops *ops, ptid_t ptid)
{
  ptid_t saved_ptid = inferior_ptid;
  struct target_ops *beneath = find_target_beneath (ops);
  int result;

  inferior_ptid = get_base_thread_from_ravenscar_task (saved_ptid);
  result = beneath->to_core_of_thread (beneath, inferior_ptid);
  inferior_ptid = saved_ptid;
  return result;
}

/* Observer on inferior_created: push ravenscar thread stratum if needed.  */

static void
ravenscar_inferior_created (struct target_ops *target, int from_tty)
{
  const char *err_msg;

  if (!ravenscar_task_support
      || gdbarch_ravenscar_ops (target_gdbarch ()) == NULL
      || !has_ravenscar_runtime ())
    return;

  err_msg = ada_get_tcb_types_info ();
  if (err_msg != NULL)
    {
      warning (_("%s. Task/thread support disabled."), err_msg);
      return;
    }

  ravenscar_update_inferior_ptid ();
  push_target (&ravenscar_ops);
}

static ptid_t
ravenscar_get_ada_task_ptid (struct target_ops *self, long lwp, long thread)
{
  return ptid_build (ptid_get_pid (base_ptid), 0, thread);
}

static void
init_ravenscar_thread_ops (void)
{
  ravenscar_ops.to_shortname = "ravenscar";
  ravenscar_ops.to_longname = "Ravenscar tasks.";
  ravenscar_ops.to_doc = "Ravenscar tasks support.";
  ravenscar_ops.to_resume = ravenscar_resume;
  ravenscar_ops.to_wait = ravenscar_wait;
  ravenscar_ops.to_fetch_registers = ravenscar_fetch_registers;
  ravenscar_ops.to_store_registers = ravenscar_store_registers;
  ravenscar_ops.to_prepare_to_store = ravenscar_prepare_to_store;
  ravenscar_ops.to_stopped_by_sw_breakpoint
    = ravenscar_stopped_by_sw_breakpoint;
  ravenscar_ops.to_stopped_by_hw_breakpoint
    = ravenscar_stopped_by_hw_breakpoint;
  ravenscar_ops.to_stopped_by_watchpoint = ravenscar_stopped_by_watchpoint;
  ravenscar_ops.to_stopped_data_address = ravenscar_stopped_data_address;
  ravenscar_ops.to_thread_alive = ravenscar_thread_alive;
  ravenscar_ops.to_update_thread_list = ravenscar_update_thread_list;
  ravenscar_ops.to_pid_to_str = ravenscar_pid_to_str;
  ravenscar_ops.to_extra_thread_info = ravenscar_extra_thread_info;
  ravenscar_ops.to_get_ada_task_ptid = ravenscar_get_ada_task_ptid;
  ravenscar_ops.to_mourn_inferior = ravenscar_mourn_inferior;
  ravenscar_ops.to_has_all_memory = default_child_has_all_memory;
  ravenscar_ops.to_has_memory = default_child_has_memory;
  ravenscar_ops.to_has_stack = default_child_has_stack;
  ravenscar_ops.to_has_registers = default_child_has_registers;
  ravenscar_ops.to_has_execution = default_child_has_execution;
  ravenscar_ops.to_stratum = thread_stratum;
  ravenscar_ops.to_core_of_thread = ravenscar_core_of_thread;
  ravenscar_ops.to_magic = OPS_MAGIC;
}

/* Command-list for the "set/show ravenscar" prefix command.  */
static struct cmd_list_element *set_ravenscar_list;
static struct cmd_list_element *show_ravenscar_list;

/* Implement the "set ravenscar" prefix command.  */

static void
set_ravenscar_command (const char *arg, int from_tty)
{
  printf_unfiltered (_(\
"\"set ravenscar\" must be followed by the name of a setting.\n"));
  help_list (set_ravenscar_list, "set ravenscar ", all_commands, gdb_stdout);
}

/* Implement the "show ravenscar" prefix command.  */

static void
show_ravenscar_command (const char *args, int from_tty)
{
  cmd_show_list (show_ravenscar_list, from_tty, "");
}

/* Implement the "show ravenscar task-switching" command.  */

static void
show_ravenscar_task_switching_command (struct ui_file *file, int from_tty,
				       struct cmd_list_element *c,
				       const char *value)
{
  if (ravenscar_task_support)
    fprintf_filtered (file, _("\
Support for Ravenscar task/thread switching is enabled\n"));
  else
    fprintf_filtered (file, _("\
Support for Ravenscar task/thread switching is disabled\n"));
}

/* Module startup initialization function, automagically called by
   init.c.  */

void
_initialize_ravenscar (void)
{
  init_ravenscar_thread_ops ();
  base_ptid = null_ptid;

  /* Notice when the inferior is created in order to push the
     ravenscar ops if needed.  */
  observer_attach_inferior_created (ravenscar_inferior_created);

  complete_target_initialization (&ravenscar_ops);

  add_prefix_cmd ("ravenscar", no_class, set_ravenscar_command,
                  _("Prefix command for changing Ravenscar-specific settings"),
                  &set_ravenscar_list, "set ravenscar ", 0, &setlist);

  add_prefix_cmd ("ravenscar", no_class, show_ravenscar_command,
                  _("Prefix command for showing Ravenscar-specific settings"),
                  &show_ravenscar_list, "show ravenscar ", 0, &showlist);

  add_setshow_boolean_cmd ("task-switching", class_obscure,
                           &ravenscar_task_support, _("\
Enable or disable support for GNAT Ravenscar tasks"), _("\
Show whether support for GNAT Ravenscar tasks is enabled"),
                           _("\
Enable or disable support for task/thread switching with the GNAT\n\
Ravenscar run-time library for bareboard configuration."),
			   NULL, show_ravenscar_task_switching_command,
			   &set_ravenscar_list, &show_ravenscar_list);
}
