# libfreertos

# 外部符号

本项目在 `FreeRTOSConfig.h` 文件中引用了外部符号。

```cpp
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
```

如果要链接 libfreertos，最终必须实现这些外部符号，否则会报链接错误。



实现了外部符号后，还要注意链接顺序。例如在 `atk-stm32f103` 库中实现了这些外部符号，则链接时，`libfreertos ` 需要在 `atk-stm32f103` 之前链接

```cmake
target_import_libfreertos(${ProjectName} PUBLIC)
target_link_libraries(
    ${ProjectName} PUBLIC
    c m
    atk-stm32f103
)

```

链接器链接 libfreertos 时，发现了未定义的符号，接着链接 atk-stm32f103，在 atk-stm32f103 中找到了这些符号的定义，于是就链接上了。



如果先链接 atk-stm32f103，再链接 libfreertos ，则链接器在处理 atk-stm32f103 中的那些实现时，发现没有地方引用它们，于是忽略了，等到链接 libfreertos 时，发现了外部符号引用，但是链接器并不会回去 atk-stm32f103 中去拿那些实现，到最后，链接器都没有找到这些符号的实现，于是报了链接错误。