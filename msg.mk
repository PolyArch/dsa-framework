define env-msg

The following environment variables must be defined
  SS_STACK (suggested: $(PWD))
  SS_TOOLS (suggested: $$SS_STACK/ss-tools)

Additionally, $$SS_TOOLS/bin must be in your $$PATH.

endef

ifeq ($(SS_TOOLS),)
$(error $(env-msg))
endif

ifeq ($(SS_STACK),)
$(error $(env-msg))
endif

ifeq ($(findstring $(SS_TOOLS)/bin,$(PATH)),)
$(error $(env-msg))
endif
