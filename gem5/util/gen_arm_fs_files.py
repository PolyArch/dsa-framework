#!/usr/bin/env python2

# Copyright (c) 2017-2018 Metempsy Technology Consulting
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Pau Cabre

from optparse import OptionParser
from subprocess import call
from platform import machine
from distutils import spawn
from glob import glob

import sys
import os

def run_cmd(explanation, working_dir, cmd, stdout = None):
    print "Running phase '%s'" % explanation
    sys.stdout.flush()

    # some of the commands need $PWD to be properly set
    env = os.environ.copy()
    env['PWD'] = working_dir

    return_code = call(cmd, cwd = working_dir, stdout = stdout,
                       env = env)

    if return_code == 0:
        return

    print "Error running phase %s. Returncode: %d" % (explanation, return_code)
    sys.exit(1)

script_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
gem5_dir = os.path.dirname(script_dir)

parser = OptionParser()

parser.add_option("--gem5-dir", default = gem5_dir,
    metavar = "GEM5_DIR",
    help = "gem5 root directory to be used for bootloader and "
           "VExpress_GEM5_V1 DTB sources. The default value is the gem5 root "
           "directory of the executed script (%default)")
parser.add_option("--dest-dir", default = "/tmp",
    metavar = "DEST_DIR",
    help = "Directory to use for checking out the different kernel "
           "repositories. Generated files will be copied to "
           "DEST_DIR/binaries (which must not exist). The default "
           "value is %default")
parser.add_option("--make-jobs", type = "int", default = 1,
    metavar = "MAKE_JOBS",
    help = "Number of jobs to use with the 'make' commands. Default value: "
           "%default")

(options, args) = parser.parse_args()

if args:
    print "Unrecognized argument(s) %s." % args
    sys.exit(1)

if not os.path.isdir(options.dest_dir):
    print "Error: %s is not a directory." % options.dest_dir
    sys.exit(1)

if not os.path.isdir(options.gem5_dir):
    print "Error: %s is not a directory." % options.gem5_dir
    sys.exit(1)

if machine() != "x86_64":
    print "Error: This script should run in a x86_64 machine"
    sys.exit(1)

binaries_dir = options.dest_dir + "/binaries"

if os.path.exists(binaries_dir):
    print "Error: %s already exists." % binaries_dir
    sys.exit(1)

revisions_dir = options.dest_dir + "/revisions"

if os.path.exists(revisions_dir):
    print "Error: %s already exists." %revisions_dir
    sys.exit(1)

# Some basic dependency checking
needed_programs = [
    "make",
    "aarch64-linux-gnu-gcc",
    "arm-linux-gnueabihf-gcc",
    "aarch64-linux-gnu-gcc-4.8",
    "arm-linux-gnueabihf-gcc-4.8",
    "gcc",
    "bc",
    "dtc",
    "arm-linux-gnueabi-gcc"
]

for program in needed_programs:
    if not spawn.find_executable(program):
        print "Error: command %s not found in $PATH" % program
        print ("If running on an Debian-based linux, please try the following "
               "cmd to get all the necessary packages: ")
        print ("sudo apt-get install -y make gcc bc gcc-aarch64-linux-gnu "
              "gcc-4.8-aarch64-linux-gnu gcc-4.8-arm-linux-gnueabihf "
              "gcc-arm-linux-gnueabihf device-tree-compiler "
              "gcc-arm-linux-gnueabi")
        sys.exit(1)

os.mkdir(binaries_dir);
os.mkdir(revisions_dir);

make_jobs_str = "-j" + str(options.make_jobs)

rev_file = open(revisions_dir + "/gem5", "w+")
run_cmd("write revision of gem5 repo",
    gem5_dir,
    ["git", "rev-parse", "--short", "HEAD"],
    rev_file)
rev_file.close()

# Checkout and build linux kernel for VExpress_GEM5_V1 (arm and arm64)
kernel_vexpress_gem5_dir = options.dest_dir + "/linux-kernel-vexpress_gem5"
run_cmd("clone linux kernel for VExpress_GEM5_V1 platform",
    options.dest_dir,
    ["git", "clone", "https://gem5.googlesource.com/arm/linux",
     kernel_vexpress_gem5_dir])
rev_file = open(revisions_dir + "/linux", "w+")
run_cmd("write revision of linux-kernel-vexpress_gem5 repo",
    kernel_vexpress_gem5_dir,
    ["git", "rev-parse", "--short", "HEAD"],
    rev_file)
rev_file.close()
run_cmd("configure kernel for arm64",
    kernel_vexpress_gem5_dir,
    ["make", "ARCH=arm64", "CROSS_COMPILE=aarch64-linux-gnu-",
     "gem5_defconfig", make_jobs_str])
run_cmd("compile kernel for arm64",
    kernel_vexpress_gem5_dir,
    ["make", "ARCH=arm64", "CROSS_COMPILE=aarch64-linux-gnu-", make_jobs_str])
run_cmd("copy arm64 vmlinux",
    kernel_vexpress_gem5_dir,
    ["cp", "vmlinux", binaries_dir + "/vmlinux.vexpress_gem5_v1_64"])
run_cmd("cleanup arm64 kernel compilation",
    kernel_vexpress_gem5_dir,
    ["make", "distclean"])
run_cmd("configure kernel for arm",
    kernel_vexpress_gem5_dir,
    ["make", "ARCH=arm", "CROSS_COMPILE=arm-linux-gnueabihf-",
     "gem5_defconfig"])
run_cmd("compile kernel for arm",
    kernel_vexpress_gem5_dir,
    ["make", "ARCH=arm", "CROSS_COMPILE=arm-linux-gnueabihf-", make_jobs_str])
run_cmd("copy arm vmlinux",
    kernel_vexpress_gem5_dir,
    ["cp", "vmlinux", binaries_dir + "/vmlinux.vexpress_gem5_v1"])

# Checkout and build linux kernel and DTB for VExpress_EMM64
kernel_vexpress_emm64_dir = options.dest_dir + "/linux-kernel-vexpress_emm64"
run_cmd("clone linux kernel for VExpress_EMM64 platform",
    options.dest_dir,
    ["git", "clone", "https://gem5.googlesource.com/arm/linux-arm64-legacy",
     kernel_vexpress_emm64_dir])
rev_file = open(revisions_dir + "/linux-arm64-legacy", "w+")
run_cmd("write revision of linux-kernel-vexpress_emm64 repo",
    kernel_vexpress_emm64_dir,
    ["git", "rev-parse", "--short", "HEAD"],
    rev_file)
rev_file.close()
run_cmd("configure kernel",
    kernel_vexpress_emm64_dir,
    ["make", "ARCH=arm64", "CROSS_COMPILE=aarch64-linux-gnu-",
     "CC=aarch64-linux-gnu-gcc-4.8", "gem5_defconfig"])
run_cmd("compile kernel",
    kernel_vexpress_emm64_dir,
    ["make", "ARCH=arm64", "CROSS_COMPILE=aarch64-linux-gnu-",
     "CC=aarch64-linux-gnu-gcc-4.8", make_jobs_str])
run_cmd("copy vmlinux",
    kernel_vexpress_emm64_dir,
    ["cp", "vmlinux", binaries_dir + "/vmlinux.vexpress_emm64"])
run_cmd("copy DTB",
    kernel_vexpress_emm64_dir,
    ["cp", "arch/arm64/boot/dts/aarch64_gem5_server.dtb", binaries_dir])

# Checkout and build linux kernel and DTBs for VExpress_EMM
kernel_vexpress_emm_dir = options.dest_dir + "/linux-kernel-vexpress_emm"
run_cmd("clone linux kernel for VExpress_EMM platform",
    options.dest_dir,
    ["git", "clone", "https://gem5.googlesource.com/arm/linux-arm-legacy",
     kernel_vexpress_emm_dir])
rev_file = open(revisions_dir + "/linux-arm-legacy", "w+")
run_cmd("write revision of linux-kernel-vexpress_emm64 repo",
    kernel_vexpress_emm_dir,
    ["git", "rev-parse", "--short", "HEAD"],
    rev_file)
rev_file.close()
run_cmd("configure kernel",
    kernel_vexpress_emm_dir,
    ["make", "ARCH=arm", "CROSS_COMPILE=arm-linux-gnueabihf-",
     "CC=arm-linux-gnueabihf-gcc-4.8", "vexpress_gem5_server_defconfig"])
run_cmd("compile kernel",
    kernel_vexpress_emm_dir,
    ["make", "ARCH=arm", "CROSS_COMPILE=arm-linux-gnueabihf-",
     "CC=arm-linux-gnueabihf-gcc-4.8", make_jobs_str])
run_cmd("copy vmlinux",
    kernel_vexpress_emm_dir,
    ["cp", "vmlinux", binaries_dir + "/vmlinux.vexpress_emm"])
run_cmd("rename DTB for 1 CPU",
    kernel_vexpress_emm_dir,
    ["cp", "arch/arm/boot/dts/vexpress-v2p-ca15-tc1-gem5.dtb",
     binaries_dir + "/vexpress-v2p-ca15-tc1-gem5_1cpus.dtb"])
run_cmd("copy DTBs",
    kernel_vexpress_emm_dir,
    ["cp"] + glob(kernel_vexpress_emm_dir + "/arch/arm/boot/dts/*gem5_*dtb") +
    [binaries_dir])

# Build DTBs for VExpress_GEM5_V1
dt_dir = gem5_dir + "/system/arm/dt"
run_cmd("compile DTBs for VExpress_GEM5_V1 platform",
    dt_dir,
    ["make", make_jobs_str])
run_cmd("copy DTBs",
    dt_dir,
    ["cp"] + glob(dt_dir + "/*dtb") + [binaries_dir])

# Build bootloaders arm64
bootloader_arm64_dir = gem5_dir + "/system/arm/aarch64_bootloader"
run_cmd("compile arm64 bootloader",
    bootloader_arm64_dir,
    ["make"])
run_cmd("copy arm64 bootloader",
    bootloader_arm64_dir,
    ["cp", "boot_emm.arm64", binaries_dir])

# Build bootloaders arm
bootloader_arm_dir = gem5_dir + "/system/arm/simple_bootloader"
run_cmd("compile arm bootloader",
    bootloader_arm_dir,
    ["make"])
run_cmd("copy arm bootloaders",
    bootloader_arm_dir,
    ["cp", "boot.arm", "boot_emm.arm", binaries_dir])

# Build m5 binaries
m5_dir = gem5_dir + "/util/m5"
run_cmd("compile arm64 m5",
    m5_dir,
    ["make", "-f", "Makefile.aarch64"])
run_cmd("copy arm64 m5",
    m5_dir,
    ["cp", "m5", binaries_dir + "/m5.aarch64"])
run_cmd("clean arm64 m5",
    m5_dir,
    ["make", "clean", "-f", "Makefile.aarch64"])
run_cmd("compile arm m5",
    m5_dir,
    ["make", "-f", "Makefile.arm"])
run_cmd("copy arm m5",
    m5_dir,
    ["cp", "m5", binaries_dir + "/m5.aarch32"])

print "Done! All the generated files can be found in %s" % binaries_dir

sys.exit(0)

