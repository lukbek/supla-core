################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/alexa/alexaclient.cpp \
../src/alexa/alexatoken.cpp 

OBJS += \
./src/alexa/alexaclient.o \
./src/alexa/alexatoken.o 

CPP_DEPS += \
./src/alexa/alexaclient.d \
./src/alexa/alexatoken.d 


# Each subdirectory must supply rules for building sources it contributes
src/alexa/%.o: ../src/alexa/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CXX) -D__DEBUG=1 -D__OPENSSL_TOOLS=1 -D__SSOCKET_WRITE_TO_FILE=$(SSOCKET_WRITE_TO_FILE) -D__BCRYPT=1 -I/usr/include/mysql -I../src/device -I../src/user -I../src -I$(SSLDIR)/include -I../src/client -O2 -g3 -Wall -c -fmessage-length=0 -fstack-protector-all -D_FORTIFY_SOURCE=2 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


