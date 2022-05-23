################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/mc_socket_r.c \
../src/msgt.pb-c.c \
../src/qfh.c \
../src/tl1.c \
../src/tl1filer.c \
../src/zmqmsg.c 

OBJS += \
./src/mc_socket_r.o \
./src/msgt.pb-c.o \
./src/qfh.o \
./src/tl1.o \
./src/tl1filer.o \
./src/zmqmsg.o 

C_DEPS += \
./src/mc_socker_t.d \
./src/msgt.pb-c.d \
./src/qfh.d \
./src/tl1.d \
./src/tl1filer.d \
./src/zmqmsg.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


