DIR=lock_managers
LOCK_MANAGERS_SOURCES=$(DIR)/global_lock_manager.cc $(DIR)/key_lock_manager.cc $(DIR)/latched_lock_manager.cc $(DIR)/latch_free_lock_manager.cc
LOCK_MANAGERS_OBJECTS=$(patsubst %.cc, %.o, $(LOCK_MANAGERS_SOURCES))

SOURCES += $(LOCK_MANAGERS_SOURCES)
