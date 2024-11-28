
TARGET := gept

C_WARNINGS := -Werror -Wall -Wlogical-op -Wextra -Wvla -Wnull-dereference \
                          -Wswitch-enum -Wno-deprecated -Wduplicated-cond -Wduplicated-branches \
                          -Wshadow -Wpointer-arith -Wcast-qual -Winit-self -Wuninitialized \
                          -Wcast-align -Wstrict-aliasing -Wformat=2 -Wmissing-declarations \
                          -Wmissing-prototypes -Wstrict-prototypes -Wwrite-strings \
                          -Wunused-parameter -Wshadow -Wdouble-promotion -Wfloat-equal \
                          -Wno-error=cpp -Wno-switch-enum -Wno-maybe-uninitialized
C_INCLUDES := -Iinclude
C_FLAGS    := $(C_WARNINGS) $(C_INCLUDES) --std=c17 -O3 -ggdb3

all:
        $(CC) $(C_FLAGS) src/gept.c -o $(TARGET)

clean:
        -rm $(TARGET)
