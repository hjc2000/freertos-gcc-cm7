#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif // _cplusplus

#include <stdint.h>
#include <stdio.h> // IWYU pragma: keep

	/* #region 接口函数 */

	///
	/// @brief 获取 SysTick 的频率
	///
	/// @note arm cortex-m 系列 CPU 有一个 systick ，里面有一个 CTRL 寄存器，其中的 bit2
	/// 可以用来控制 systick 的时钟源。
	/// 	@li 为 1 时表示使用与 CPU 相同的时钟源，即 systick 的频率会与 CPU 相同。
	/// 	@li 为 0 则表示不要求 systick 的频率与 CPU 相同。
	///
	/// 所以 bit2 可以理解为同步控制位，置 1 后会让 systick 时钟与 CPU 同步。
	///
	/// @note 是否让 systick 同步到 CPU 频率是 freertos 控制的。详见下面的 SYNC_TO_CPU 宏定义。
	///
	/// @note 注意：有的单片机无法让 systick 使用与 CPU 不同频率的时钟源。例如 stm32h743，
	/// 就算你将 systick 的 CTRL 寄存器的 bit2 设置为 0 ，它也会使用与 CPU 相同的系统时钟源，
	/// 不会对它进行 8 分频。虽然 cubemx 的时钟树上显示出了 8 分频，但是实际上是分不了的，
	/// 配置好 8 分频后, cubemx 生成的代码中也没有关于 8 分频的代码。而且，经过实测，无论将 bit2
	/// 设置成什么, systick 的频率一直等于系统时钟的频率，即与 CPU 同频率。
	///
	/// @param sync_to_cpu 是否同步到 CPU
	/// 	@note 为 true 表示要获取 SysTick 同步到 CPU 频率时的频率，也即希望获取 CPU 频率。
	///
	/// 	@note 为 false 表示要获取的是 SysTick 不同步到 CPU 时的频率。例如对于 stm32f103，就是
	/// 	获取系统时钟 8 分频后的频率。（系统时钟是 CPU 的时钟源，系统时钟频率等于 CPU 频率）
	///
	/// @return SysTick 在 sync_to_cpu 指示的模式下的频率。
	/// 	@note 如果 sync_to_cpu 为 true ，返回 CPU 频率。
	/// 	@note 如果 sync_to_cpu 为 false，返回与 CPU 频率不同的那个频率。
	///
	uint32_t freertos_get_systic_clock_freq(uint8_t sync_to_cpu);

	///
	/// @brief 位于 libfreertos.a 中的一个函数，并没有暴露到头文件中。
	/// 用户需要在 SysTick 的溢出中断处理函数中调用。在 arm 工具链的启动文件中，
	/// 这个函数被汇编确定为 SysTick_Handler ，用户需要定义这个函数并实现为类似：
	///
	/*

		void SysTick_Handler()
		{
			HAL_IncTick();
			if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
			{
				xPortSysTickHandler();
			}
		}

	 */
	///
	void xPortSysTickHandler();

	/* #endregion */

/* 1: 抢占式调度器, 0: 协程式调度器, 无默认需定义 */
#define configUSE_PREEMPTION 1

/* 1: 使用硬件计算下一个要运行的任务, 0: 使用软件算法计算下一个要运行的任务, 默认: 0 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* 1: 使能 tickless 低功耗模式, 默认: 0 */
#define configUSE_TICKLESS_IDLE 0

/* 是否让 systick 的频率同步到 CPU 频率。 */
#define SYNC_TO_CPU 1

#if SYNC_TO_CPU
	#define configCPU_CLOCK_HZ freertos_get_systic_clock_freq(1)
#else
	#define configSYSTICK_CLOCK_HZ freertos_get_systic_clock_freq(0)
#endif

/* 定义系统时钟节拍频率, 单位: Hz, 无默认需定义 */
#define configTICK_RATE_HZ 1000

/* 定义最大优先级数, 最大优先级 = configMAX_PRIORITIES - 1, 无默认需定义 */
#define configMAX_PRIORITIES 56

/* 定义空闲任务的栈空间大小, 单位: 字。 无默认需定义 */
#define configMINIMAL_STACK_SIZE 512

/* 定义任务名最大字符数, 默认: 16 */
#define configMAX_TASK_NAME_LEN 16

	///
	/// @brief 1: 定义系统时钟节拍计数器的数据类型为 16 位无符号数, 无默认需定义。
	///
	/// @note 定义为 0 则 freertos 会使用 uint32_t 作为 TickType_t 的数据类型，
	/// 定义为 1 则 freertos 会使用 uint16_t 作为 TickType_t 的数据类型。
	///
	///
#define configUSE_16_BIT_TICKS 0

	///
	/// @brief 1: 使能在抢占式调度下, 同优先级的任务能抢占空闲任务, 默认: 1
	///
	///
#define configIDLE_SHOULD_YIELD 1

/* 1: 使能任务间直接的消息传递,包括信号量、事件标志组和消息邮箱, 默认: 1 */
#define configUSE_TASK_NOTIFICATIONS 1

/* 定义任务通知数组的大小, 默认: 1 */
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1

	///
	/// @brief 1: 使能互斥锁, 默认: 0
	///
	///
#define configUSE_MUTEXES 1

	///
	/// @brief 1: 使能递归互斥锁, 默认: 0
	///
	///
#define configUSE_RECURSIVE_MUTEXES 1

	///
	/// @brief 1: 使能计数信号量, 默认: 0
	///
	///
#define configUSE_COUNTING_SEMAPHORES 1

/* 1: 使能兼容老版本, 默认: 1 */
#define configENABLE_BACKWARD_COMPATIBILITY 0

/* 定义任务堆栈深度的数据类型, 默认: uint16_t */
#define configSTACK_DEPTH_TYPE uint16_t

/* 定义消息缓冲区中消息长度的数据类型, 默认: size_t */
#define configMESSAGE_BUFFER_LENGTH_TYPE size_t

	/* #region 内存分配相关 */

/* 1: 支持动态申请内存, 默认: 1 */
#define configSUPPORT_DYNAMIC_ALLOCATION 1

/* FreeRTOS 堆中可用的 RAM 总量, 单位: Byte, 无默认需定义 */
#define configTOTAL_HEAP_SIZE ((size_t)(200 * 1024))

	/* #endregion */

	/* #region 钩子函数 */

/* 1: 使能空闲任务钩子函数, 无默认需定义  */
#define configUSE_IDLE_HOOK 0

/* 1: 使能系统时钟节拍中断钩子函数, 无默认需定义 */
#define configUSE_TICK_HOOK 0

/* 1: 使能栈溢出检测方法1, 2: 使能栈溢出检测方法2, 默认: 0 */
#define configCHECK_FOR_STACK_OVERFLOW 1

/* 1: 使能动态内存申请失败钩子函数, 默认: 0 */
#define configUSE_MALLOC_FAILED_HOOK 0

	/* 1: 使能定时器服务任务首次执行前的钩子函数, 默认: 0 */
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

	/* #endregion */

	/* #region 运行时间和任务状态统计 */

	///
	/// @brief 1: 使能任务运行时间统计功能, 默认: 0
	///
	///
#define configGENERATE_RUN_TIME_STATS 0

#if configGENERATE_RUN_TIME_STATS
	#include "./BSP/TIMER/btim.h"
	#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() ConfigureTimeForRunTimeStats()
	extern uint32_t FreeRTOSRunTimeTicks;
	#define portGET_RUN_TIME_COUNTER_VALUE() FreeRTOSRunTimeTicks
#endif

	///
	/// @brief 1: 使能可视化跟踪调试, 默认: 0
	///
	///
#define configUSE_TRACE_FACILITY 1

	///
	/// @brief 1: configUSE_TRACE_FACILITY为1时，会编译vTaskList()和vTaskGetRunTimeStats()函数, 默认: 0
	///
	///
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

	/* #endregion */

	/* #region 软件定时器相关定义 */

#define configUSE_TIMERS 1                                          /* 1: 使能软件定时器, 默认: 0 */
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)        /* 定义软件定时器任务的优先级, 无默认configUSE_TIMERS为1时需定义 */
#define configTIMER_QUEUE_LENGTH 5                                  /* 定义软件定时器命令队列的长度, 无默认configUSE_TIMERS为1时需定义 */
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2) /* 定义软件定时器任务的栈空间大小, 无默认configUSE_TIMERS为1时需定义 */

	/* #endregion */

	/* #region 可选功能 */

/* 设置任务优先级 */
#define INCLUDE_vTaskPrioritySet 1

/* 获取任务优先级 */
#define INCLUDE_uxTaskPriorityGet 1

/* 删除任务 */
#define INCLUDE_vTaskDelete 1

/* 挂起任务 */
#define INCLUDE_vTaskSuspend 1

/* 恢复在中断中挂起的任务 */
#define INCLUDE_xResumeFromISR 1

/* 任务绝对延时 */
#define INCLUDE_vTaskDelayUntil 1

/* 任务延时 */
#define INCLUDE_vTaskDelay 1

// 获取调度器运行状态
#define INCLUDE_xTaskGetSchedulerState 1

/* 获取当前任务的任务句柄 */
#define INCLUDE_xTaskGetCurrentTaskHandle 1

/* 获取任务堆栈历史剩余最小值 */
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/* 获取空闲任务的任务句柄 */
#define INCLUDE_xTaskGetIdleTaskHandle 1

/* 获取任务状态 */
#define INCLUDE_eTaskGetState 1

/* 在中断中设置事件标志位 */
#define INCLUDE_xEventGroupSetBitFromISR 1

/* 将函数的执行挂到定时器服务任务 */
#define INCLUDE_xTimerPendFunctionCall 1

/* 中断任务延时 */
#define INCLUDE_xTaskAbortDelay 1

/* 通过任务名获取任务句柄 */
#define INCLUDE_xTaskGetHandle 1

/* 恢复在中断中挂起的任务 */
#define INCLUDE_xTaskResumeFromISR 1

#define INCLUDE_xSemaphoreGetMutexHolder 1

	/* #endregion */

	/* #region 中断优先级 */

/*
 * 设置为 HAL 库中的 __NVIC_PRIO_BITS 的值。
 * 如果查找符号，找到了很多个，可以将 __NVIC_PRIO_BITS 放到源文件中，
 * 然后 F12 导航到定义。
 */
#define configPRIO_BITS 4

/* 中断最低优先级 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15

/* FreeRTOS可管理的最高中断优先级 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_API_CALL_INTERRUPT_PRIORITY configMAX_SYSCALL_INTERRUPT_PRIORITY

	/* #endregion */

	/* #region FreeRTOS 中断服务函数 */

	///
	/// @brief 定义成与启动文件中中断向量表所引用的函数一样的名称。
	/// HAL 提供的启动文件中是 PendSV_Handler
	///
	///
#define xPortPendSVHandler PendSV_Handler

	///
	/// @brief 定义成与启动文件中中断向量表所引用的函数一样的名称。
	/// HAL 提供的启动文件中是 SVC_Handler
	///
	///
	///
#define vPortSVCHandler SVC_Handler

	/* #endregion */

	/* #region 断言 */

#define vAssertCalled(char, int) printf("freertos 内部发生了错误: %s, %d\r\n", char, int)

#define configASSERT(x)                    \
	if ((x) == 0)                          \
	{                                      \
		vAssertCalled(__FILE__, __LINE__); \
	}

	/* #endregion */

#ifdef __cplusplus
}
#endif // __cplusplus
#endif /* FREERTOS_CONFIG_H */
