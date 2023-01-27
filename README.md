goldfish/	: directory containing all the modified kernel code.
	arch/arm/
		mm/fault.c
		configs/goldfish_armv7_defconfig
	include/linux/
		sched.h
		init_task.h
	kernel/sched/
		core.c
		fair.c
		ras.c : the major implementation of scheduler RAS.
		sched.h
		Makefile

system_call/	: directory containing the implementation of system call 361, 362, 363.
	start_trace/
		start_trace.c : the implementation of system call start_trace.
		Makefile : make configurations of "start_trace.c".
	stop_trace/
		stop_trace.c : the implementation of system call stop_trace.
		Makefile
	get_trace/
		get_trace.c : the implementation of system call get_trace.
		Makefile

test/	: directory containing the test processes.
	mem_test/
		jni/
			mem_test.c : source code for testing the page access tracing mechanism.
			Android.mk : ndk-build configurations of "mem_test.c".
		mem_test_testscript : testscript of mem_test.c
	set_sched/
		jni/
			set_sched.c : source code for changing the scheduler of the specified process.
			Android.mk
		set_sched_testscript : testscript of set_sched.c
	multiprocess/
		jni/
			multiprocess.c : source code for testing RAS scheduler with several processes.
			Android.mk
		multiprocess_testscript : testscript of multiprocess.c
	exec_time/jni/
		exec_time.c : source code for compare the runtime(performance) of different schedulers.
		Android.mk

OS_Project2_Report.pdf : report of this project.

