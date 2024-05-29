#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#ifdef __cplusplus
extern "C" {
	#endif // _cplusplus

	#include<stdint.h>
	#include<stdio.h>

	/// <summary>
	///		获取 CPU 频率
	/// </summary>
	/// <returns></returns>
	uint32_t freertos_get_cpu_freq();

	/// <summary>
	///		获取硬件 systic 时钟频率。是最终输入计数器的那个频率，不是指分频器前面的那个。
	/// </summary>
	/// <returns></returns>
	uint32_t freertos_get_systic_clock_freq();

	/* 1: 抢占式调度器, 0: 协程式调度器, 无默认需定义 */
	#define configUSE_PREEMPTION                            1

	/* 1: 使用硬件计算下一个要运行的任务, 0: 使用软件算法计算下一个要运行的任务, 默认: 0 */
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION         1

	/* 1: 使能tickless低功耗模式, 默认: 0 */
	#define configUSE_TICKLESS_IDLE                         0

	/* 定义CPU主频, 单位: Hz, 无默认需定义 */
	#define configCPU_CLOCK_HZ								freertos_get_cpu_freq()
	#define configSYSTICK_CLOCK_HZ                          freertos_get_systic_clock_freq()

	/* 定义系统时钟节拍频率, 单位: Hz, 无默认需定义 */
	#define configTICK_RATE_HZ                              1000

	/* 定义最大优先级数, 最大优先级=configMAX_PRIORITIES-1, 无默认需定义 */
	#define configMAX_PRIORITIES                            32

	/* 定义空闲任务的栈空间大小, 单位: Word, 无默认需定义 */
	#define configMINIMAL_STACK_SIZE                        128

	/* 定义任务名最大字符数, 默认: 16 */
	#define configMAX_TASK_NAME_LEN                         16

	/* 1: 定义系统时钟节拍计数器的数据类型为16位无符号数, 无默认需定义 */
	#define configUSE_16_BIT_TICKS                          0

	/* 1: 使能在抢占式调度下,同优先级的任务能抢占空闲任务, 默认: 1 */
	#define configIDLE_SHOULD_YIELD                         1

	/* 1: 使能任务间直接的消息传递,包括信号量、事件标志组和消息邮箱, 默认: 1 */
	#define configUSE_TASK_NOTIFICATIONS                    1

	/* 定义任务通知数组的大小, 默认: 1 */
	#define configTASK_NOTIFICATION_ARRAY_ENTRIES           1

	/* 1: 使能互斥锁, 默认: 0 */
	#define configUSE_MUTEXES                               1

	/* 1: 使能递归互斥锁, 默认: 0 */
	#define configUSE_RECURSIVE_MUTEXES                     1

	/* 1: 使能计数信号量, 默认: 0 */
	#define configUSE_COUNTING_SEMAPHORES                   1

	/* 已弃用!!! */
	#define configUSE_ALTERNATIVE_API                       0

	/* 定义可以注册的信号量和消息队列的个数, 默认: 0 */
	#define configQUEUE_REGISTRY_SIZE                       8

	/* 1: 使能队列集, 默认: 0 */
	#define configUSE_QUEUE_SETS                            1

	/* 1: 使能时间片调度, 默认: 1 */
	#define configUSE_TIME_SLICING                          1

	/* 1: 任务创建时分配Newlib的重入结构体, 默认: 0 */
	#define configUSE_NEWLIB_REENTRANT                      0

	/* 1: 使能兼容老版本, 默认: 1 */
	#define configENABLE_BACKWARD_COMPATIBILITY             0

	/* 定义线程本地存储指针的个数, 默认: 0 */
	#define configNUM_THREAD_LOCAL_STORAGE_POINTERS         0

	/* 定义任务堆栈深度的数据类型, 默认: uint16_t */
	#define configSTACK_DEPTH_TYPE                          uint16_t

	/* 定义消息缓冲区中消息长度的数据类型, 默认: size_t */
	#define configMESSAGE_BUFFER_LENGTH_TYPE                size_t

	#pragma region 内存分配相关
	/* 1: 支持静态申请内存, 默认: 0 */
	#define configSUPPORT_STATIC_ALLOCATION                 0

	/* 1: 支持动态申请内存, 默认: 1 */
	#define configSUPPORT_DYNAMIC_ALLOCATION                1

	/* FreeRTOS堆中可用的RAM总量, 单位: Byte, 无默认需定义 */
	#define configTOTAL_HEAP_SIZE                           ((size_t)(20 * 1024))

	/* 1: 用户手动分配FreeRTOS内存堆(ucHeap), 默认: 0 */
	#define configAPPLICATION_ALLOCATED_HEAP                0

	/* 1: 用户自行实现任务创建时使用的内存申请与释放函数, 默认: 0 */
	#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP       0
	#pragma endregion

	#pragma region 钩子函数
	/* 1: 使能空闲任务钩子函数, 无默认需定义  */
	#define configUSE_IDLE_HOOK                             0

	/* 1: 使能系统时钟节拍中断钩子函数, 无默认需定义 */
	#define configUSE_TICK_HOOK                             0

	/* 1: 使能栈溢出检测方法1, 2: 使能栈溢出检测方法2, 默认: 0 */
	#define configCHECK_FOR_STACK_OVERFLOW                  0

	/* 1: 使能动态内存申请失败钩子函数, 默认: 0 */
	#define configUSE_MALLOC_FAILED_HOOK                    0

	/* 1: 使能定时器服务任务首次执行前的钩子函数, 默认: 0 */
	#define configUSE_DAEMON_TASK_STARTUP_HOOK              0
	#pragma endregion

	#pragma region 运行时间和任务状态统计
	/* 1: 使能任务运行时间统计功能, 默认: 0 */
	#define configGENERATE_RUN_TIME_STATS                   0
	#if configGENERATE_RUN_TIME_STATS
	#include "./BSP/TIMER/btim.h"
	#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()        ConfigureTimeForRunTimeStats()
	extern uint32_t FreeRTOSRunTimeTicks;
	#define portGET_RUN_TIME_COUNTER_VALUE()                FreeRTOSRunTimeTicks
	#endif
	/* 1: 使能可视化跟踪调试, 默认: 0 */
	#define configUSE_TRACE_FACILITY                        1
	/* 1: configUSE_TRACE_FACILITY为1时，会编译vTaskList()和vTaskGetRunTimeStats()函数, 默认: 0 */
	#define configUSE_STATS_FORMATTING_FUNCTIONS            1
	#pragma endregion

	#pragma region 协程
	/* 1: 启用协程, 默认: 0 */
	#define configUSE_CO_ROUTINES                           0

	/* 定义协程的最大优先级, 最大优先级=configMAX_CO_ROUTINE_PRIORITIES-1,
	* 无默认configUSE_CO_ROUTINES为1时需定义
	*/
	#define configMAX_CO_ROUTINE_PRIORITIES                 2
	#pragma endregion

	/* 软件定时器相关定义 */
	#define configUSE_TIMERS                                1                               /* 1: 使能软件定时器, 默认: 0 */
	#define configTIMER_TASK_PRIORITY                       ( configMAX_PRIORITIES - 1 )    /* 定义软件定时器任务的优先级, 无默认configUSE_TIMERS为1时需定义 */
	#define configTIMER_QUEUE_LENGTH                        5                               /* 定义软件定时器命令队列的长度, 无默认configUSE_TIMERS为1时需定义 */
	#define configTIMER_TASK_STACK_DEPTH                    ( configMINIMAL_STACK_SIZE * 2) /* 定义软件定时器任务的栈空间大小, 无默认configUSE_TIMERS为1时需定义 */

	#pragma region 可选功能
	/* 设置任务优先级 */
	#define INCLUDE_vTaskPrioritySet                        1

	/* 获取任务优先级 */
	#define INCLUDE_uxTaskPriorityGet                       1

	/* 删除任务 */
	#define INCLUDE_vTaskDelete                             1

	/* 挂起任务 */
	#define INCLUDE_vTaskSuspend                            1

	/* 恢复在中断中挂起的任务 */
	#define INCLUDE_xResumeFromISR                          1

	/* 任务绝对延时 */
	#define INCLUDE_vTaskDelayUntil                         1

	/* 任务延时 */
	#define INCLUDE_vTaskDelay                              1

	// 获取调度器运行状态
	#define INCLUDE_xTaskGetSchedulerState                  1

	/* 获取当前任务的任务句柄 */
	#define INCLUDE_xTaskGetCurrentTaskHandle               1

	/* 获取任务堆栈历史剩余最小值 */
	#define INCLUDE_uxTaskGetStackHighWaterMark             1

	/* 获取空闲任务的任务句柄 */
	#define INCLUDE_xTaskGetIdleTaskHandle                  1

	/* 获取任务状态 */
	#define INCLUDE_eTaskGetState                           1

	/* 在中断中设置事件标志位 */
	#define INCLUDE_xEventGroupSetBitFromISR                1

	/* 将函数的执行挂到定时器服务任务 */
	#define INCLUDE_xTimerPendFunctionCall                  1

	/* 中断任务延时 */
	#define INCLUDE_xTaskAbortDelay                         1

	/* 通过任务名获取任务句柄 */
	#define INCLUDE_xTaskGetHandle                          1

	/* 恢复在中断中挂起的任务 */
	#define INCLUDE_xTaskResumeFromISR                      1
	#pragma endregion

	#pragma region 中断优先级
	/*
	* 设置为 HAL 库中的 __NVIC_PRIO_BITS 的值。
	* 如果查找符号，找到了很多个，可以将 __NVIC_PRIO_BITS 放到源文件中，
	* 然后 F12 导航到定义。
	*/
	#define configPRIO_BITS									4

	/* 中断最低优先级 */
	#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

	/* FreeRTOS可管理的最高中断优先级 */
	#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
	#define configKERNEL_INTERRUPT_PRIORITY                 ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
	#define configMAX_SYSCALL_INTERRUPT_PRIORITY            ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
	#define configMAX_API_CALL_INTERRUPT_PRIORITY           configMAX_SYSCALL_INTERRUPT_PRIORITY
	#pragma endregion

	#pragma region FreeRTOS中断服务函数
	/* 定义成与启动文件中中断向量表所引用的函数一样的名称。HAL 提供的启动文件中是 PendSV_Handler */
	#define xPortPendSVHandler		PendSV_Handler

	/* 定义成与启动文件中中断向量表所引用的函数一样的名称。HAL 提供的启动文件中是 SVC_Handler */
	#define vPortSVCHandler			SVC_Handler
	#pragma endregion

	/* 断言 */
	#define vAssertCalled(char, int) printf("发生了错误: %s, %d\r\n", char, int)
	#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )




	#ifdef __cplusplus
}
#endif // __cplusplus
#endif /* FREERTOS_CONFIG_H */
