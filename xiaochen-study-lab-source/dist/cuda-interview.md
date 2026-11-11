# CUDA 面试篇

> **本地学习整理：**内容来自 [霍墨墨 · AI Infra Notes](https://hd9568.github.io/topics/cuda/)。原文著作权归原作者所有，本专题保留每篇文章的原始链接，仅供个人学习复习。

围绕 CUDA 面试与 GPU 算子优化整理，覆盖基础模型、内存体系、并发执行、性能诊断、手写算子以及 CUTLASS/CuTe 等进阶主题。

## 1 高频面试题总集

### CUDA 高频面试题 340 问：从硬件数值到 Blackwell 算子优化

> 原文：[CUDA 高频面试题 340 问：从硬件数值到 Blackwell 算子优化](https://hd9568.github.io/blog/cuda-interview-questions-300/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**按问题与答案整理 340 道 CUDA 高频面试题，覆盖线程模型、内存体系、同步并发、性能分析、硬件资源计算、GEMM、Tensor Core、Ampere/Hopper/Blackwell 与大模型算子。

#### 使用方式

本文不是按篇幅展开概念，而是模拟面试中的连续追问。答案先给结论，再用一两句话解释原因；涉及寄存器数、Shared Memory 容量和最大驻留数量时，以目标 GPU 的 Compute Capability 与 `cudaDeviceProp` 为准，不把某一代硬件参数当作 CUDA 永久规则。

#### 一、基础语法与编译模型：Q1-Q25

##### Q1：CUDA 是什么？

**答：** CUDA 是 NVIDIA 提供的并行计算平台和编程模型，使程序可以在 GPU 上执行通用计算。它不仅是一门语言，还包含编译器、Runtime/Driver API、数学库、调试与性能分析工具。

##### Q2：CUDA 和 GPU 是一回事吗？

**答：** 不是。GPU 是硬件，CUDA 是控制 NVIDIA GPU 执行并行程序的软件平台和编程模型；同一块 GPU 还可以通过图形 API 或其他计算接口使用。

##### Q3：Host 和 Device 分别指什么？

**答：** Host 通常指 CPU 及其内存，Device 指 GPU 及其显存。CUDA 程序由 Host 发起内存管理、数据传输和 Kernel Launch，Device 执行并行 Kernel。

##### Q4：Kernel 是什么？

**答：** Kernel 是在 GPU 上由大量线程并行执行的函数，使用 `__global__` 修饰。一次 Kernel Launch 会创建一个 Grid，Grid 中每个线程执行同一份函数代码，但拥有不同索引和私有状态。

##### Q5：`__global__`、`__device__` 和 `__host__` 有什么区别？

**答：** `__global__` 函数由 Host 或支持动态并行的 Device 发起、在 Device 执行；`__device__` 只能在 Device 调用和执行；`__host__` 在 CPU 调用和执行。`__host__ __device__` 会为两个目标各编译一个版本。

##### Q6：Kernel Launch 中的四个尖括号参数是什么？

**答：** `kernel<<<grid, block, shared_bytes, stream>>>()` 依次指定 Grid 维度、每个 Block 的线程维度、动态 Shared Memory 字节数和执行 Stream。后两项可以省略。

##### Q7：`dim3` 是什么？

**答：** `dim3` 是保存 `x/y/z` 三个无符号维度的辅助类型，用于描述 Grid 和 Block。未指定的维度默认为 1，因此 `dim3 block(256)` 等价于 `(256,1,1)`。

##### Q8：如何计算一维 Global Thread ID？

**答：** 常用公式是 `idx = blockIdx.x * blockDim.x + threadIdx.x`。Grid 往往向上取整，因此访问数组前还要判断 `idx < n`。

##### Q9：如何把二维 Block 内线程线性化？

**答：** `linear_tid = threadIdx.x + blockDim.x * (threadIdx.y + blockDim.y * threadIdx.z)`。CUDA 按 x 最快、然后 y、最后 z 的顺序线性化，这也决定线程如何被划入 Warp。

##### Q10：为什么 Grid 和 Block 支持三维？

**答：** 主要为了自然表达图像、矩阵和三维网格的索引，维度本身不会自动提升性能。最终线程仍被线性化并按 Warp 调度。

##### Q11：Kernel Launch 对 Host 是同步还是异步？

**答：** 一般对 Host 异步，Launch 将工作加入 Stream 后即可返回。Launch 配置错误可以立即发现，越界等执行错误通常要在后续同步点才能报告。

##### Q12：如何检查 Kernel Launch 错误？

**答：** Launch 后用 `cudaGetLastError()` 或 `cudaPeekAtLastError()` 检查配置错误，再在调试阶段对 `cudaDeviceSynchronize()` 的返回值进行检查以发现异步执行错误。

##### Q13：`cudaGetLastError` 和 `cudaPeekAtLastError` 有什么区别？

**答：** 两者都读取当前线程的 Runtime Error State，但 `cudaGetLastError` 会清除该错误，`cudaPeekAtLastError` 不清除。错误检查宏中必须明确是否希望消费错误状态。

##### Q14：CUDA Runtime API 和 Driver API 有什么区别？

**答：** Runtime API 更高层，自动管理 Primary Context，常见前缀是 `cuda`；Driver API 更底层，显式管理 Context、Module 和 Function，前缀是 `cu`。两者最终使用同一驱动，可在满足 Context 规则时互操作。

##### Q15：NVCC 做了什么？

**答：** NVCC 将 Host 代码交给 Host Compiler，将 Device 代码编译为 PTX 和/或目标架构的机器码 SASS，再把它们封装进 Fat Binary。程序运行时驱动选择可直接执行的 Cubin，或对 PTX 做 JIT。

##### Q16：PTX 和 SASS 有什么区别？

**答：** PTX 是与具体芯片解耦的虚拟 ISA，便于驱动 JIT 到未来架构；SASS 是某个真实 GPU 架构执行的机器指令。分析最终性能必须看 SASS，而不能只看 PTX。

##### Q17：`-arch` 和 `-code` 分别控制什么？

**答：** `-arch=compute_xy` 指定生成 Device Code 时使用的虚拟架构能力，`-code=sm_xy` 或 `compute_xy` 指定 Fatbin 中保存目标 Cubin 还是 PTX。工程中通常同时嵌入常用 SM 的 Cubin 和一个 PTX 回退。

##### Q18：什么是 Compute Capability？

**答：** Compute Capability 用主次版本表示 GPU 支持的硬件与指令特性，例如原子类型、Shared Memory 上限、Tensor Core 和 Cluster 能力。代码应按能力查询或编译分派，不能只按产品名称判断。

##### Q19：Cubin 能跨 GPU 架构执行吗？

**答：** Cubin 通常只在二进制兼容范围内执行，不能假设跨主 Compute Capability 兼容。PTX 具有向后 JIT 能力，但新 PTX 也要求驱动足够新。

##### Q20：为什么第一次 Kernel Launch 可能更慢？

**答：** 可能包含 CUDA Context 初始化、Module Lazy Loading、PTX JIT、内存页建立和库内部 Autotune。基准测试需要先 Warmup，不能把首次初始化时间算入稳态 Kernel 延迟。

##### Q21：`__forceinline__` 一定更快吗？

**答：** 不一定。内联可消除调用开销并暴露优化机会，但也可能扩大代码、增加寄存器生命周期和 Instruction Cache 压力，应查看编译结果与实测。

##### Q22：`__restrict__` 的作用是什么？

**答：** 它承诺该指针访问的对象不会与其他受限指针别名，使编译器能更自由地缓存、重排 Load/Store。若实际发生别名，程序行为可能错误，因此不能只把它当性能开关。

##### Q23：`constexpr` 和模板参数为什么常用于 CUDA Kernel？

**答：** 它们让 Tile Size、Stage 数等在编译期已知，便于循环展开、静态 Shared Memory 分配和删除无效分支。代价是产生更多 Kernel 实例并增加编译时间和二进制体积。

##### Q24：JIT Kernel 的优缺点是什么？

**答：** JIT 可以针对运行时 Shape、数据类型和 GPU 架构生成专用代码，减少通用分支；缺点是首次编译延迟、缓存管理复杂和线上编译环境要求更高。

##### Q25：为什么不能只记住某张 GPU 的硬件上限？

**答：** 最大线程数、寄存器、Shared Memory、Cluster 约束会随架构和具体 SKU 变化。面试中应先讲资源约束关系，再说明实际值通过 Programming Guide、`cudaDeviceProp` 或 Occupancy API 查询。

#### 二、线程、Warp、CTA 与 SM：Q26-Q55

##### Q26：Thread、Block 和 Grid 的关系是什么？

**答：** Thread 是逻辑线程，多个 Thread 构成 Block，多个 Block 构成一次 Kernel 的 Grid。Block 内可以用 Shared Memory 和块级同步协作，普通 Block 之间不能假设执行顺序。

##### Q27：CTA 和 Thread Block 是什么关系？

**答：** CTA 是 Cooperative Thread Array，与 CUDA Thread Block 基本同义。CTA 强调这组线程共享资源并协作执行，也是硬件调度到 SM 的资源分配单位。

##### Q28：SM 是什么？

**答：** SM 是 GPU 上执行 CTA 和 Warp 的硬件处理器，内部包含 Warp Scheduler、寄存器文件、Shared Memory/L1、Load/Store 单元、CUDA Core 和 Tensor Core 等资源。

##### Q29：CTA 和 SM 的关系是什么？

**答：** 一个普通 CTA 一生只驻留在一个 SM 上，不会执行到一半迁移；一个 SM 可以在资源允许时同时驻留多个 CTA。CTA 数量多于可同时驻留数量时会分成多波执行。

##### Q30：一个 CTA 能跨多个 SM 吗？

**答：** 普通 CTA 不能。Hopper 以后 Thread Block Cluster 可让多个 CTA 分别驻留在多个 SM 上协作，但 Cluster 中每个 CTA 仍各自属于一个 SM。

##### Q31：Warp 是什么？

**答：** Warp 是 NVIDIA GPU 的线程调度基本单位，当前 CUDA 架构通常每个 Warp 有 32 个 Lane。Warp 中线程共享指令发射，但每个 Lane 有自己的寄存器、地址和 Active Mask。

##### Q32：SIMT 和 SIMD 有什么区别？

**答：** SIMD 由一条显式向量指令操作多个元素，SIMT 向程序员暴露多个独立线程，再由硬件将它们组成 Warp 执行。两者都利用并行数据通路，但 SIMT 保留线程级控制流和状态。

##### Q33：Warp 中线程必须一直执行同一条路径吗？

**答：** 不必须，但分歧路径通常要在不同 Active Mask 下分别执行，导致部分 Lane 暂时空闲。Volta 之后有 Independent Thread Scheduling，但这没有消除 Divergence 成本。

##### Q34：什么是 Warp Divergence？

**答：** 同一 Warp 的线程因数据相关分支走不同路径，硬件需要分时执行这些路径并屏蔽不参与的 Lane。不同 Warp 走不同分支不叫 Warp Divergence。

##### Q35：所有 `if` 都会导致严重 Divergence 吗？

**答：** 不会。若整个 Warp 条件一致，或者编译器将短分支转成 Predication，代价可能很小；只有同一 Warp 内路径不一致且分支工作较重时影响明显。

##### Q36：什么是 Predication？

**答：** 编译器可能让两条短路径都按谓词指令执行，仅对条件满足的 Lane 提交结果，从而避免真正跳转。它减少控制流开销，但被屏蔽指令仍可能消耗发射资源。

##### Q37：什么是 Active Mask？

**答：** Active Mask 表示当前 Warp 中哪些 Lane 正参与指令。Warp Vote、Shuffle 和 `__syncwarp` 应使用与实际参与线程一致的 Mask，错误使用完整 Mask 可能造成未定义行为或死锁。

##### Q38：`lane_id` 和 `warp_id` 如何计算？

**答：** 对线性线程号 `tid`，`lane_id = tid % warpSize`，`warp_id = tid / warpSize`。代码可用 `warpSize` 而不是硬编码 32，尽管当前 NVIDIA CUDA Warp 通常为 32。

##### Q39：Warp Scheduler 如何隐藏延迟？

**答：** 当一个 Warp 等待 Global Memory 或依赖结果时，Scheduler 可发射另一个 Ready Warp 的指令。隐藏延迟依赖足够的 Ready Warp 或单 Warp 内足够的 ILP，而不只是线程总数。

##### Q40：什么是 Occupancy？

**答：** Occupancy 是每个 SM 实际可驻留 Warp 数与架构最大 Warp 数之比。它描述可用于隐藏延迟的并发 Warp 容量，不等于 CUDA Core 或 Tensor Core 的实际利用率。

##### Q41：Occupancy 越高越好吗？

**答：** 不是。达到足以隐藏延迟的 Occupancy 后，继续压低寄存器或缩小 Tile 可能减少数据复用和 ILP，反而变慢；最终以吞吐和延迟实测为准。

##### Q42：哪些资源限制每个 SM 的 Resident CTA 数？

**答：** 主要包括每 CTA 线程/Warps、每线程寄存器、每 CTA Shared Memory，以及架构允许的最大 CTA、Warp 和线程数。最终 Resident CTA 数由最先耗尽的资源决定。

##### Q43：寄存器如何限制 Occupancy？

**答：** 每个 SM 的寄存器文件有限，`每线程寄存器数 × 每 CTA 线程数 × 驻留 CTA 数` 不能超过上限，而且分配还受粒度影响。寄存器增加可能减少可同时驻留的 CTA。

##### Q44：Shared Memory 如何限制 Occupancy？

**答：** 每个 CTA 的静态和动态 Shared Memory 都从 SM 容量中分配。若单 CTA 使用量过大，SM 即使还有线程和寄存器配额，也可能只能驻留一个 CTA。

##### Q45：什么是 Wave Quantization？

**答：** Grid 中 CTA 按 SM 可驻留容量分波执行，最后一波若 CTA 很少会使部分 SM 空闲。例如一波可运行 132 个 CTA，而最后只剩 20 个，就会产生明显 Tail。

##### Q46：什么是 Persistent Kernel？

**答：** Persistent Kernel 只启动接近 SM 驻留容量的 CTA，每个 CTA 在循环中持续领取多个任务。它便于统一调度不规则工作、减少重复 Launch，并可通过动态任务队列缓解负载不均。

##### Q47：Persistent CTA 能完全消除 Tail 吗？

**答：** 不能。它只有在多个小任务形成统一任务池或支持动态领取时才明显减少尾部；如果总任务本来少于 SM 数，Persistent 也无法创造额外并行度。

##### Q48：Block 数小于 SM 数会怎样？

**答：** 最多只有同样数量的 SM 获得工作，其余 SM 空闲。可尝试减小每 Block 工作量、拆分 Reduction 维或提高 Batch 并行度，但拆得过细会增加 Launch、同步和中间结果成本。

##### Q49：Block 数越多越好吗？

**答：** 不是。足够覆盖所有 SM 并隐藏 Tail 后，继续增加 Block 可能只增加重复加载、原子竞争和调度开销；任务粒度要结合数据复用与负载均衡选择。

##### Q50：如何选择每个 Block 的线程数？

**答：** 通常从 128、256 等 Warp 整数倍开始，结合寄存器、Shared Memory、每线程工作量和数据布局用 Occupancy API 与基准测试调优。1024 线程虽然可能合法，却经常限制 Resident CTA 数。

##### Q51：Block 中线程数必须是 32 的倍数吗？

**答：** 不是语义要求，但通常建议如此。否则最后一个 Warp 有无效 Lane，除非边界问题或算法结构使这种浪费可以接受。

##### Q52：不同 Block 的执行顺序有保证吗？

**答：** 普通 Kernel 没有保证，不能用 Block ID 推断先后。跨 Block 依赖应拆成多个 Kernel、使用 Cooperative Launch/Grid Sync，或设计正确的全局原子协议。

##### Q53：什么是 Thread Block Cluster？

**答：** Cluster 是 Hopper 引入的 CTA 分组，保证一组 CTA 在受支持的硬件范围内协同调度。Cluster 可使用 Cluster Barrier、Distributed Shared Memory 和 TMA Multicast。

##### Q54：什么是 Cooperative Launch？

**答：** Cooperative Launch 保证整个 Grid 的 Block 能按要求同时驻留，从而允许 `grid.sync()`。由于必须满足全 Grid Residency，Grid 大小和 Occupancy 受到更严格限制。

##### Q55：Dynamic Parallelism 是什么？

**答：** 它允许 Device Kernel 启动新的 Kernel，适合递归或动态工作生成。其 Launch 和同步成本不低，规则也更复杂，许多场景用 Host 调度或 Persistent Work Queue 更高效。

#### 三、CUDA 内存体系：Q56-Q85

##### Q56：CUDA 主要有哪些内存空间？

**答：** 常见有 Register、Local、Shared、Global、Constant、Texture，以及硬件管理的 L1/L2 Cache。它们的物理位置、作用域、生命周期、容量和访问模式不同。

##### Q57：Register 的作用域和生命周期是什么？

**答：** Register 通常是线程私有的片上状态，生命周期随线程执行结束。它延迟低但总量有限，使用过多会降低 Occupancy 或发生 Spilling。

##### Q58：Local Memory 在哪里？

**答：** Local Memory 在逻辑上线程私有，物理上位于 Device Memory，并经过 L1/L2 Cache。大数组、动态索引对象或寄存器不足都可能使变量进入 Local Memory。

##### Q59：什么是 Register Spilling？

**答：** 编译器无法把所有线程私有值放进寄存器时，会生成 Local Load/Store。Spill 增加显存流量和延迟，但少量缓存命中的 Spill 不一定立即成为主要瓶颈。

##### Q60：如何判断发生了 Spilling？

**答：** 编译时查看 `ptxas -v` 的 spill load/store 与 stack frame，运行时用 Nsight Compute 检查 Local Memory 事务和相关 Stall。源码变量数量本身不能准确判断。

##### Q61：Shared Memory 是什么？

**答：** Shared Memory 是 CTA 内线程共享的可编程片上 Scratchpad，生命周期与 CTA 相同。它常用于数据复用、线程间交换和重排访问模式。

##### Q62：Global Memory 是什么？

**答：** Global Memory 是所有 GPU 线程可访问的大容量 Device Memory，通常对应显存 HBM/GDDR，并经过 Cache。其带宽高但延迟远高于片上存储，需要合并访问和足够并行度。

##### Q63：L1 和 Shared Memory 是什么关系？

**答：** 在多代架构上它们共享或紧密耦合同一片片上存储资源，但具体容量划分和配置随架构变化。Shared Memory 由程序显式管理，L1 由硬件 Cache 策略管理。

##### Q64：L2 Cache 的作用域是什么？

**答：** L2 是 Device 级共享 Cache，服务所有 SM，并处于 Global Memory 之前。跨 SM 的只读复用、原子操作和部分写回都会经过 L2，但命中并不由程序完全保证。

##### Q65：Constant Memory 适合什么访问？

**答：** 适合同一 Warp 的线程读取同一个地址，此时 Constant Cache 可广播一次数据。若每个 Lane 读取不同 Constant 地址，请求可能被串行化。

##### Q66：Texture Memory 适合什么访问？

**答：** Texture 路径适合只读且具有二维空间局部性、地址模式不规则或需要硬件采样功能的数据。对普通连续数组，现代只读 Cache 或正常 Global Load 可能已经足够。

##### Q67：Static Shared Memory 和 Dynamic Shared Memory 有何区别？

**答：** Static Shared Memory 大小在编译期确定，如 `__shared__ float s[256]`；Dynamic Shared Memory 通过 `extern __shared__` 声明，大小由 Launch 的第三个参数传入。

##### Q68：多个 Dynamic Shared Memory 数组如何组织？

**答：** `extern __shared__` 实际提供一段字节区，需要手工切片并满足每种类型的对齐。错误的 Offset 或对齐会造成未对齐访问甚至结果错误。

##### Q69：`cudaMalloc` 分配的内存是否自动初始化？

**答：** 不会，内容未定义。需要零初始化时使用 `cudaMemset`，但注意它按字节填充值，不能用来设置任意浮点数。

##### Q70：`cudaMemset(ptr, 1, bytes)` 能把 `float` 数组设成 1 吗？

**答：** 不能，它把每个字节写成 `0x01`，得到的浮点位模式不是 `1.0f`。零值位模式全零，因此设 0 通常可用。

##### Q71：`cudaMallocPitch` 解决什么问题？

**答：** 它为二维数据分配满足硬件对齐的 Row Pitch，可能在每行尾部加入 Padding。Kernel 必须使用返回的 Pitch 计算行地址，不能假设等于 `width * sizeof(T)`。

##### Q72：Pinned Host Memory 是什么？

**答：** Pinned Memory 是不会被 OS 换出的页锁定 Host Memory，DMA 引擎可直接传输。它通常提高 H2D/D2H 带宽并支持真正的异步重叠，但大量锁页会损害系统内存管理。

##### Q73：Pageable Host Memory 为什么通常更慢？

**答：** DMA 需要稳定物理页，Runtime 往往先把 Pageable Buffer 复制到内部 Pinned Staging Buffer，再发起传输。额外 CPU Copy 和同步使延迟增大。

##### Q74：Unified Memory 是什么？

**答：** `cudaMallocManaged` 提供 CPU/GPU 可访问的统一指针，Runtime 根据访问迁移或映射页面。它简化编程，但缺页迁移和不良访问模式可能造成严重抖动。

##### Q75：如何优化 Unified Memory？

**答：** 使用 `cudaMemPrefetchAsync` 提前迁移，配合 `cudaMemAdvise` 声明首选位置和访问者，并避免 CPU/GPU 高频交替写同一批页面。仍应通过 NSYS 查看 Page Fault 与迁移行为。

##### Q76：UVA 和 Unified Memory 是一回事吗？

**答：** 不是。Unified Virtual Addressing 让 Host 和多个 Device 的地址位于统一虚拟地址空间，便于识别指针归属；Unified Memory 还负责数据可访问性与页面迁移。

##### Q77：Zero-copy Memory 是什么？

**答：** 将 Pinned Host Memory 映射到 Device 地址后，GPU 可直接访问 Host Memory，无需显式复制。它适合一次性、低复用和小规模访问，频繁随机访问会受 PCIe/NVLink 延迟限制。

##### Q78：Device Memory Pool 有什么作用？

**答：** Memory Pool 复用已分配的 Device Memory，减少高频 `cudaMalloc/cudaFree` 的分配开销和潜在同步。`cudaMallocAsync/cudaFreeAsync` 还能按 Stream 顺序管理生命周期。

##### Q79：`cudaMallocAsync` 为什么更适合并发流水？

**答：** 它把分配操作纳入 Stream 依赖，而不是要求全设备安全点，并从 Pool 中复用内存。正确性仍要求使用该内存的 Stream 依赖在 Free 之前完成。

##### Q80：`cudaFree` 可能带来什么问题？

**答：** 传统 `cudaFree` 可能触发隐式同步或昂贵的分配器路径，破坏流水。高频临时 Buffer 更适合预分配、Memory Pool 或 `cudaFreeAsync`。

##### Q81：Device Pointer 能在 Host 上直接解引用吗？

**答：** 普通 `cudaMalloc` 返回的 Device Pointer 不能由 CPU 直接读写。UVA 统一了地址形式，不代表每个地址对 CPU 都可访问。

##### Q82：Host Pointer 能直接传给 Kernel 吗？

**答：** 普通 `malloc` 指针通常不能。Managed Memory、Mapped Pinned Memory 或支持系统分配访问的特定平台可以，但要满足设备能力和映射规则。

##### Q83：为什么大模型推理经常预分配 Workspace？

**答：** 预分配避免请求期间反复分配、同步和碎片，也让地址稳定以支持 CUDA Graph。代价是需要估算上限并设计复用与并发隔离。

##### Q84：显存碎片和“总空闲够但分配失败”是什么关系？

**答：** 分配器可能找不到足够大的连续可用块，或内存被不同生命周期的 Buffer 切碎。Pool、分层 Arena、固定大小 Page 和 Paged KV Cache 都是在缓解碎片。

##### Q85：为什么不能按固定延迟给内存类型排序？

**答：** 实际延迟受 Cache 命中、冲突、合并访问、并行请求和架构影响。正确表述是理解作用域与数据路径，再通过 Profiling 判断当前 Kernel 的实际瓶颈。

#### 四、合并访问、Shared Memory 与 Cache：Q86-Q115

##### Q86：什么是 Global Memory Coalescing？

**答：** 一个 Warp 的内存请求若能被硬件合并成尽量少的对齐 Memory Transaction，就称为合并访问。重点不是地址在源码里“看起来连续”，而是最终触及多少内存 Sector。

##### Q87：连续线程访问连续 `float` 为什么通常高效？

**答：** 32 个 Lane 共访问连续 128 字节，通常只覆盖少量连续且对齐的 Sector，取回数据利用率高。具体 Transaction 组织随 Cache 配置和架构变化。

##### Q88：Stride 访问为什么可能浪费带宽？

**答：** 若 Lane `i` 访问 `base + i * stride`，大 Stride 会让一个 Warp 触及许多 Sector，每个 Sector 只使用少量字节。有效带宽因过量传输下降。

##### Q89：未对齐访问一定很慢吗？

**答：** 不一定，但跨越额外 Sector 时会增加 Transaction；相邻 Warp 可能从 Cache 复用多取的数据。应结合 Requested Bytes、Transferred Bytes 和 Sector 数判断。

##### Q90：`float4` 一定比 `float` 快吗？

**答：** 不一定。向量化能减少指令数并改善宽 Load/Store，但要求地址和数据长度满足对齐，且可能增加寄存器压力；访存已合并时收益可能有限。

##### Q91：AoS 和 SoA 哪个更适合 GPU？

**答：** 若 Warp 同时读取同一字段，Structure of Arrays 往往使该字段连续并更易合并；若每个线程总是读取完整小对象，AoS 也可能合理。选择取决于 Warp 的字段访问模式。

##### Q92：矩阵转置为什么容易出现非合并写？

**答：** 直接让线程读取一行再写一列时，读或写的一侧会形成大 Stride。常见方法是先以合并方式读入 Shared Memory，再交换索引以合并方式写出。

##### Q93：Shared Memory 有多少个 Bank？

**答：** 当前主流 NVIDIA 架构通常有 32 个 Bank，连续 32-bit Word 映射到连续 Bank。精确 Bank 宽度和特殊模式应按目标架构文档确认。

##### Q94：什么是 Bank Conflict？

**答：** 同一 Warp 的一个 Shared Memory 请求访问同一 Bank 中的不同地址时，硬件需要拆成多次服务，形成 Conflict。Conflict 度越高，完成请求所需 Wavefront 越多。

##### Q95：同一 Warp 的线程读取同一个 Shared 地址会冲突吗？

**答：** 通常不会按普通冲突串行，而会使用 Broadcast/Multicast 机制返回同一个值。冲突针对的是同一 Bank 的不同地址。

##### Q96：为什么转置 Tile 常写成 `[32][33]`？

**答：** `[32][32]` 按列访问时相邻 Lane 的地址可能都映射到同一 Bank；多一列改变行 Stride 的模 32 关系，使相邻 Lane 分散到不同 Bank。

##### Q97：Padding 总能消除 Bank Conflict 吗？

**答：** 不能。它只对特定 Element Size、二维 Stride 和访问方向有效，向量 Load、64/128-bit 访问会产生更复杂的 Bank 组合，需要根据实际地址和 NCU 指标验证。

##### Q98：Shared Memory Swizzle 是什么？

**答：** Swizzle 用位运算重排 Shared Memory 坐标，使 Tensor Core 或转置访问分散到不同 Bank，同时保持可逆的逻辑布局。它比固定 `+1` Padding 更适合复杂 Tile。

##### Q99：Shared Memory 为什么能提高 GEMM 性能？

**答：** CTA 将 A/B Tile 从 Global Memory 加载一次后，可被多个线程和多次 FMA 复用。它用片上容量换取更高 Arithmetic Intensity。

##### Q100：使用 Shared Memory 一定更快吗？

**答：** 不一定。若数据只用一次、L1 已充分命中，额外写 Shared、同步和地址计算可能更贵；Shared 还会限制 Occupancy。

##### Q101：什么是 Tiling？

**答：** Tiling 把大问题切成适合片上存储与线程层级的小块，在 Tile 内进行数据复用。高性能 Kernel 往往同时有 CTA Tile、Warp Tile、MMA Tile 和 Thread Tile。

##### Q102：为什么 Tile 越大不一定越好？

**答：** 大 Tile 增加数据复用和每 CTA 工作量，但消耗更多 Shared Memory、寄存器并减少并行 CTA 数，还可能增加边界浪费。应在复用、Occupancy 和并行度之间平衡。

##### Q103：什么是 Double Buffering？

**答：** 使用两组片上 Buffer，让一组参与计算时另一组加载下一 Tile。其目标是重叠数据搬运和计算，而不是减少总传输字节。

##### Q104：多 Stage Pipeline 和 Double Buffer 有什么关系？

**答：** Double Buffer 是两 Stage 的特例。增加 Stage 可以覆盖更长的访存延迟，但会消耗更多 Shared Memory 和 Barrier 状态，Stage 过多可能降低 Occupancy。

##### Q105：`cp.async` 解决什么问题？

**答：** Ampere 的 `cp.async` 可把数据从 Global Memory 异步复制到 Shared Memory，减少通过普通寄存器中转的 Load/Store 指令，并允许 Copy 与计算形成 Pipeline。

##### Q106：TMA 和 `cp.async` 的主要区别是什么？

**答：** TMA 从 Hopper 起可由少量线程通过 Tensor Descriptor 发起 1D 到多维大块搬运，并由硬件完成地址生成；`cp.async` 通常由 Warp 多线程协作搬运较小片段。

##### Q107：TMA 为什么适合 Warp Specialization？

**答：** 单个选举线程即可发起大块异步传输，Producer Warp 不必用全部 Lane 做地址计算和 Load/Store，其他 Warp 可专职执行 WGMMA 或 Epilogue。

##### Q108：什么是 TMA Multicast？

**答：** Cluster 中多个 CTA 需要同一 Tile 时，一次 TMA 操作可把数据发送到多个 CTA 的 Shared Memory，减少 HBM/L2 重复读取。它要求 Cluster 和 Barrier 正确配置。

##### Q109：L1 Cache Miss 一定会访问 HBM 吗？

**答：** 不一定，先查询 Device 级 L2，命中后无需访问 HBM。只有下层 Cache 也未命中或写回需要时才产生显存流量。

##### Q110：Cache 命中率越高性能越好吗？

**答：** 不一定。高命中率可能对应很少的有效工作，或 Kernel 瓶颈在计算和同步；应同时看吞吐、Sector 数、请求字节和执行时间。

##### Q111：什么时候使用 L2 Persisting Cache？

**答：** 当一小段数据会跨 Kernel 或多 CTA 高频复用时，可通过 Access Policy Window 提示 L2 保留。设置范围过大或多个 Stream 竞争会降低效果。

##### Q112：`__ldg` 还有必要吗？

**答：** 在旧架构上它显式走只读数据路径；现代编译器通常能根据 `const __restrict__` 自动选择合适 Load。需要检查目标架构和生成代码，不应机械添加。

##### Q113：怎样计算 Arithmetic Intensity？

**答：** `AI = 有效 FLOPs / 从目标内存层传输的字节数`。必须说明分母针对 HBM、L2 还是 Shared Memory，因为不同层对应不同 Roofline。

##### Q114：如何判断一个优化是否真正减少 HBM 流量？

**答：** 从算法上计算理论字节数，再用 Nsight Compute 检查 DRAM Bytes/Sectors。源码中加入 Shared Memory 不代表 HBM 一定减少，Cache 与重复加载方式会影响实际流量。

##### Q115：为什么边界 Mask 会影响访存效率？

**答：** 尾部 Warp 只有少数 Lane 有效，却可能仍触发完整 Sector 和指令，降低有效字节比例。Padding、专门 Tail Kernel 或 Predication 可改善，但要与额外复杂度权衡。

#### 五、同步、原子操作与内存模型：Q116-Q145

##### Q116：`__syncthreads()` 做什么？

**答：** 它是 CTA 范围的 Barrier，所有参与线程到达后才能继续，并使 Barrier 前的相关 Shared/Global Memory 访问对 CTA 内线程可见。它不是全 Grid 同步。

##### Q117：把 `__syncthreads()` 放在分支里有什么风险？

**答：** 如果同一 CTA 中不是所有线程都执行到同一个 Barrier，部分线程会永久等待，造成死锁。只有条件对整个 CTA 一致时才安全。

##### Q118：`__syncwarp()` 做什么？

**答：** 它同步指定 Mask 中的 Warp Lane，并为这些线程提供内存有序性。Mask 必须只包含会实际到达该调用的线程。

##### Q119：Volta 以后还可以假设 Warp 隐式同步吗？

**答：** 不应该。Independent Thread Scheduling 允许 Lane 进度更独立，Warp 内通过 Shared/Global Memory 通信时应使用 `__syncwarp` 或正式的 Cooperative Groups 原语。

##### Q120：Warp Shuffle 有什么作用？

**答：** `__shfl*_sync` 让 Lane 直接交换寄存器值，适合 Warp Reduction、Broadcast 和 Scan，避免 Shared Memory 与块级 Barrier。它只能在指定 Active Mask 内交换。

##### Q121：`__ballot_sync` 返回什么？

**答：** 它收集指定 Warp Lane 的布尔谓词，返回一个 Bit Mask。可用于计算满足条件的线程数、Warp 内压缩和分支状态分析。

##### Q122：`__activemask()` 可以直接作为之前分支的参与 Mask 吗？

**答：** 不总是。它只反映调用这一刻仍活跃的 Lane，可能已因调度和控制流变化而丢失原参与集合；更稳妥的是在分支前用 `__ballot_sync` 明确生成 Mask。

##### Q123：Atomic Operation 保证什么？

**答：** 它保证指定作用域内对某个地址的读改写不可被其他同类访问撕裂。原子性不等于全局 Barrier，也不自动提供所需的内存顺序。

##### Q124：为什么大量 `atomicAdd` 会慢？

**答：** 多线程竞争同一地址时操作必须序列化，并产生 Cache Line/Atomic Unit 争用。常用优化是 Warp 或 Block 内先归约，再减少全局原子次数。

##### Q125：Atomic 一定比锁快吗？

**答：** 不一定，但 GPU 上简单原子通常比自旋锁更安全。锁持有线程若未被调度，而同一 Warp 或占满资源的其他线程持续自旋，可能造成严重停顿甚至死锁。

##### Q126：如何用 `atomicCAS` 实现自定义原子操作？

**答：** 循环读取旧值、计算新值，再用 CAS 尝试替换；失败说明被其他线程修改，需要重试。要处理浮点 NaN、ABA 和内存顺序等边界。

##### Q127：`atomicAdd` 能代替 Reduction 吗？

**答：** 正确性上常可行，性能取决于竞争度和硬件原子吞吐。大规模 Reduction 通常先做 Warp/Block 局部归约，最后每 Block 只原子一次。

##### Q128：什么是 Memory Fence？

**答：** Fence 约束调用线程的内存操作在指定作用域内的可见顺序，但不等待其他线程到达。它需要与标志位、原子或 Barrier 配合构造同步协议。

##### Q129：`__threadfence_block`、`__threadfence`、`__threadfence_system` 有何区别？

**答：** 它们分别面向 Block、Device 和 System 作用域建立调用线程的内存顺序。作用域越大通常成本越高，应选择满足正确性的最小范围。

##### Q130：`volatile` 能代替 Atomic 或 Fence 吗？

**答：** 不能。`volatile` 主要限制编译器对访问的缓存和消除，不保证多线程原子性，也不完整定义跨线程同步顺序。

##### Q131：什么是 Data Race？

**答：** 多线程并发访问同一位置，至少一个是写，并且缺少满足内存模型的同步时产生 Data Race。结果可能不确定，不能依赖“多数运行正确”。

##### Q132：如何检测 CUDA Race？

**答：** 使用 Compute Sanitizer 的 `racecheck` 检查 Shared Memory Hazard，结合 `memcheck`、`synccheck` 和最小化输入定位。工具不能证明所有并发程序无 Race。

##### Q133：为什么 Kernel 之间天然可以作为全局同步点？

**答：** 同一 Stream 中后一个 Kernel 必须等前一个完成，因此前一 Kernel 的全局写入对后一 Kernel 可见。两次 Launch 的成本换来了简单可靠的 Grid 级阶段边界。

##### Q134：如何在一个 Kernel 内做 Grid Sync？

**答：** 使用 Cooperative Groups 的 `this_grid().sync()`，并通过 Cooperative Launch 启动。必须保证整个 Grid 可同时驻留，否则无法安全完成全 Grid Barrier。

##### Q135：Cooperative Groups 的价值是什么？

**答：** 它把协作线程组显式表示为对象，支持 Block、Warp Tile、Cluster 和 Grid 等不同粒度的同步、Reduction 与 Scan，避免依赖脆弱的隐式 Warp 假设。

##### Q136：`cuda::barrier` 和 `__syncthreads` 有何不同？

**答：** `cuda::barrier` 可分离 Arrive 与 Wait，并与异步 Copy 的完成计数结合，更适合 Producer-Consumer Pipeline；`__syncthreads` 是一次固定的全 CTA 到达等待。

##### Q137：什么是 Producer-Consumer Pipeline？

**答：** Producer 异步填充某个 Stage，Consumer 等待该 Stage Ready 后计算，再通知 Buffer 可复用。多 Stage 通过 Phase/Barrier 避免覆盖尚未消费的数据。

##### Q138：什么是 Memory Order Relaxed？

**答：** Relaxed Atomic 只保证该原子对象的原子修改顺序，不为其他普通 Load/Store 建立同步关系。计数器常可用 Relaxed，发布数据通常需要 Release/Acquire。

##### Q139：Release/Acquire 解决什么问题？

**答：** Producer 先写数据再 Release 发布标志，Consumer Acquire 读到该标志后，可以观察 Producer 在 Release 前的写入。前提是作用域和内存位置匹配。

##### Q140：CUDA 为什么需要 Thread Scope？

**答：** 同步线程距离不同，成本和一致性点也不同。Block、Device、System 等 Scope 让程序只为实际通信范围支付成本。

##### Q141：Cluster Scope 是什么？

**答：** 它位于 Block 与 Device 之间，覆盖同一 Thread Block Cluster 内的 CTA。Cluster Barrier、Distributed Shared Memory 和 Cluster Atomic 可在该范围协作。

##### Q142：Distributed Shared Memory 是什么？

**答：** Hopper Cluster 中，一个 CTA 可以访问同 Cluster 其他 CTA 的 Shared Memory 地址。它仍是片上受限资源，远端访问和同步成本高于本 CTA Shared Memory。

##### Q143：两个 CTA 能直接用 `__syncthreads()` 同步吗？

**答：** 不能，`__syncthreads` 只覆盖当前 CTA。Cluster CTA 使用 Cluster Barrier，任意 Grid CTA 则需 Cooperative Grid Sync、Atomic 协议或拆 Kernel。

##### Q144：为什么自旋等待可能导致 GPU 死锁？

**答：** 等待方 CTA 可能占满全部 Resident 资源，使负责设置条件的 CTA 无法被调度；Warp 内锁持有者和等待者的发散执行也可能互相阻塞。应避免依赖不保证调度公平的协议。

##### Q145：如何选择同步粒度？

**答：** 使用满足正确性的最小线程范围：能用 Warp 就不做 CTA Barrier，能用 CTA 就不做 Grid/Device 同步。范围越大，等待线程越多且内存一致性成本越高。

#### 六、Stream、Event、异步拷贝与 CUDA Graph：Q146-Q175

##### Q146：CUDA Stream 是什么？

**答：** Stream 是按提交顺序执行的 Device 工作队列，可包含 Kernel、Memcpy、Memset、Event 等操作。同一 Stream 内有顺序保证，不同 Stream 的工作在硬件资源允许时可以重叠。

##### Q147：不同 Stream 的 Kernel 一定并发吗？

**答：** 不一定。它们只是具备并发资格，还受 SM 资源、Kernel Occupancy、Hyper-Q、依赖关系和默认 Stream 语义限制；一个 Kernel 占满全部 SM 资源时另一个无法驻留。

##### Q148：Legacy Default Stream 有什么特殊语义？

**答：** Legacy Default Stream 会与同一 Context 中的普通 Blocking Stream 形成隐式同步，容易破坏并发。Non-blocking Stream 和 Per-thread Default Stream 的语义不同，不能笼统说“Stream 0 总会同步一切”。

##### Q149：Per-thread Default Stream 是什么？

**答：** 每个 Host Thread 拥有自己的默认 Stream，并像普通 Stream 一样与其他 Stream 并发，避免 Legacy 全局默认流的隐式同步。可通过编译选项或宏启用。

##### Q150：Kernel 和 Host 代码如何异步？

**答：** Kernel Launch 通常只把命令写入 Stream 后返回，CPU 可继续工作。若随后调用同步 API、同步 Memcpy 或访问依赖结果，Host 才会等待。

##### Q151：`cudaMemcpyAsync` 为什么可能没有重叠？

**答：** Host Buffer 不是 Pinned、Copy 和 Kernel 在同一 Stream、设备缺少相应 Copy Engine，或存在隐式同步时都可能无法重叠。必须用 Nsight Systems 验证时间线。

##### Q152：实现 H2D、Kernel、D2H Pipeline 需要什么？

**答：** 把数据分 Chunk，使用 Pinned Host Buffer 和多个非默认 Stream，让不同 Chunk 分别处于 H2D、计算和 D2H 阶段。设备还需支持 Copy/Compute Concurrent Capability。

##### Q153：为什么数据分 Chunk 才容易重叠？

**答：** 单个大 Buffer 必须完整 H2D 后 Kernel 才能开始，也要完整计算后才能 D2H。Chunking 建立生产流水，使第 `i+1` 块传输时第 `i` 块可计算。

##### Q154：Stream Priority 能保证高优先级 Kernel 立即抢占吗？

**答：** 不能保证细粒度抢占正在运行的 CTA。Priority 主要影响待调度工作选择，实际抢占能力和粒度随硬件与工作类型变化。

##### Q155：CUDA Event 有什么用途？

**答：** Event 可在 Stream 中记录进度，用于跨 Stream 依赖、异步完成查询和 GPU 时间测量。它比全设备同步粒度更小。

##### Q156：`cudaStreamWaitEvent` 为什么有用？

**答：** 它让一个 Stream 等待另一个 Stream 中的 Event，而 Host 无需阻塞。这样能在 Device 侧构建 DAG 依赖并保留其他独立工作的并发。

##### Q157：为什么用 CUDA Event 测 Kernel 时间？

**答：** Event 时间戳记录在 Device 执行时间线上，可排除 Host 异步 Launch 返回造成的误差。开始和结束 Event 应记录在同一目标 Stream 并等待结束 Event 完成。

##### Q158：CUDA Event 计时包含排队时间吗？

**答：** 包含两个 Event 在对应 Stream 时间线之间的等待和执行，若期间有依赖或其他同 Stream 工作也会计入。它不是纯指令执行周期分析工具。

##### Q159：`cudaDeviceSynchronize` 和 `cudaStreamSynchronize` 有何区别？

**答：** 前者等待当前 Device 的相关工作，后者只等待指定 Stream。性能敏感代码应优先使用满足依赖的最小同步范围。

##### Q160：`cudaStreamQuery` 有什么作用？

**答：** 它非阻塞检查 Stream 是否完成，返回 `cudaErrorNotReady` 表示仍在执行。适合 Host 轮询或与其他 CPU 工作协同。

##### Q161：什么是 CUDA Graph？

**答：** CUDA Graph 把一组 Kernel、Memcpy 和依赖预先实例化为可重复提交的执行图，减少每轮 Host Launch 与依赖解析开销。它特别适合 Shape 和地址稳定的迭代工作负载。

##### Q162：CUDA Graph 为什么适合 LLM Decode？

**答：** Decode 每步有大量短 Kernel，GPU 计算时间可能接近 CPU Launch 时间，而且执行拓扑重复。Graph 将一整步批量提交，减少 CPU Gap。

##### Q163：CUDA Graph 会让单个 Kernel 算得更快吗？

**答：** 通常不会改变 Kernel 内部算法，主要减少 Host 调度和 Launch 间隙。若原瓶颈是大 GEMM 计算，Graph 收益可能很小。

##### Q164：Stream Capture 是什么？

**答：** Runtime 记录 Capture 期间提交到 Stream 及其依赖 Stream 的操作，生成 Graph。Capture 期间某些同步、分配和不安全 API 受限制。

##### Q165：Graph 中输入数据能变化吗？

**答：** Buffer 内容可以变化，但默认情况下节点参数和地址按实例化时记录。可保持固定地址写入新内容，或使用 Graph Update/Node Parameter Update 修改允许的参数。

##### Q166：为什么动态 Shape 给 CUDA Graph 带来困难？

**答：** Shape 变化可能改变 Grid、Workspace、Kernel 选择和内存地址。常见方案是按 Shape Bucket 建多个 Graph，或固定最大 Shape 并传入 Device Mask/有效长度。

##### Q167：Graph Capture 中为什么避免普通 `cudaMalloc`？

**答：** 传统分配可能涉及全局状态和同步，不一定可安全捕获。通常预分配稳定 Workspace，或使用支持 Graph/Stream 顺序的 Memory Pool API。

##### Q168：什么是 Programmatic Dependent Launch？

**答：** PDL 允许后继 Kernel 在前驱 Kernel 完全结束前被调度，并在真正依赖数据前等待，以重叠前驱尾部和后继准备阶段。它需要硬件、Launch 属性和 Kernel 协议支持。

##### Q169：Host Callback 能做什么？

**答：** Callback/Host Function 在 Stream 到达指定位置后由 Host 执行，适合通知和轻量后处理。回调中不应调用可能导致 CUDA 死锁的 API，也不适合重 CPU 任务。

##### Q170：多个 Host Thread 使用 CUDA 有什么注意事项？

**答：** 要理解 Primary Context、当前 Device、Per-thread Default Stream 和 Runtime Error State 的线程语义。共享 Stream、Event 和 Buffer 时仍需 Host 侧同步。

##### Q171：一个进程可以创建多少 Stream？

**答：** API 可创建很多 Stream，但硬件并发队列、SM 和 Copy Engine 数量有限。Stream 过多会增加管理成本，不会线性增加并发。

##### Q172：Stream 顺序能保证内存可见性吗？

**答：** 同一 Stream 中后续操作观察前序操作结果；跨 Stream 必须通过 Event、显式同步或正确内存模型建立依赖。仅按 Host 提交先后不能保证不同 Stream 的 Device 执行顺序。

##### Q173：库调用如何绑定 Stream？

**答：** cuBLAS、cuDNN 等 Handle 通常需要显式设置 Stream。若库仍使用默认 Stream，可能破坏框架已有并发和依赖。

##### Q174：为什么频繁创建销毁 Stream/Event 不好？

**答：** 它增加 Host Runtime 管理和资源分配开销。长期服务通常建立 Stream/Event Pool 并重复使用。

##### Q175：怎样确认多 Stream 真的有效？

**答：** 用 Nsight Systems 查看不同 Stream 的 Kernel/Copy 是否在时间线上重叠，并比较端到端吞吐。只看 API 已使用 `Async` 或创建了多个 Stream 不能证明并发。

#### 七、性能模型、Occupancy 与 Profiling：Q176-Q210

##### Q176：CUDA Kernel 常见瓶颈类型有哪些？

**答：** 常见有 Compute-bound、Memory-bandwidth-bound、Memory-latency-bound、Launch-bound、 Synchronization-bound 和 Instruction-bound。优化前必须先识别主瓶颈。

##### Q177：Compute-bound 是什么？

**答：** 算术或 Tensor Core 吞吐接近上限，而内存带宽仍有余量，增加计算单元效率比减少少量访存更重要。典型例子是足够大的高 Arithmetic Intensity GEMM。

##### Q178：Memory-bandwidth-bound 是什么？

**答：** Kernel 主要时间花在搬运数据，DRAM/L2 吞吐接近可达上限，计算单元利用率较低。融合、量化、复用和减少中间写回通常有效。

##### Q179：Memory-latency-bound 和 Bandwidth-bound 有何区别？

**答：** Latency-bound 可能带宽不高，但请求少、依赖链长或并行 Warp 不足，无法隐藏单次访问延迟；Bandwidth-bound 则已有大量请求并接近持续吞吐上限。

##### Q180：Launch-bound 是什么？

**答：** Kernel 很短且数量很多，Host Launch、Driver 调度和 Kernel 间空隙占据显著时间。Kernel Fusion、CUDA Graph 和减少同步比优化单个 Kernel 算术更有效。

##### Q181：Roofline Model 怎么用？

**答：** 根据 Arithmetic Intensity 与硬件计算峰值、内存带宽的交点估计性能上界。实测远低于两条 Roof 时，可能还有 Occupancy、依赖、同步或指令效率问题。

##### Q182：理论 FLOPs 怎么计算？

**答：** 例如 GEMM `C[M,N]=A[M,K]B[K,N]` 通常按 `2MNK` FLOPs 计数，因为一次乘加算两次浮点操作。必须说明是否包括 Bias、Activation 和稀疏零跳过。

##### Q183：理论带宽和有效带宽怎么区分？

**答：** 理论带宽来自硬件频率与总线，实测 DRAM 带宽来自硬件计数器；应用有效带宽按算法必要字节除时间计算。非合并访问可能使 DRAM 很忙而有效带宽很低。

##### Q184：什么是 Achieved Occupancy？

**答：** 它根据运行时活跃 Warp 采样得到，可能低于静态理论 Occupancy。短 Kernel、Tail、Barrier 和负载不均都会影响实际值。

##### Q185：什么是 Theoretical Occupancy？

**答：** 根据 Block Size、寄存器、Shared Memory 和硬件上限计算每 SM 可驻留 Warp 比例。它是资源可行性估算，不描述 Warp 是否 Ready 或执行单元是否忙。

##### Q186：如何计算理论 Occupancy？

**答：** 先分别计算线程/Warps、寄存器、Shared Memory、最大 CTA 对 Resident CTA 的限制，取最小值，再换算 Resident Warp/最大 Warp。实际还要考虑资源分配粒度。

##### Q187：Occupancy 低时第一步做什么？

**答：** 先确认它是否真的限制性能，再查限制来源是寄存器、Shared Memory 还是 Block Size。盲目追求更高 Occupancy 可能牺牲 ILP 和数据复用。

##### Q188：什么是 Register Pressure？

**答：** 活跃值太多使每线程寄存器需求升高，可能降低 Resident Warp 或 Spill。循环展开、大 Thread Tile 和复杂 Epilogue 都会延长寄存器生命周期。

##### Q189：`-maxrregcount` 是万能优化吗？

**答：** 不是。它可迫使编译器减少寄存器以提高 Occupancy，但可能产生大量 Spill；应比较限制前后的 Local Memory 事务和执行时间。

##### Q190：`__launch_bounds__` 有什么作用？

**答：** 它向编译器声明最大 Block Threads 和期望每 SM 最小 CTA 数，帮助寄存器分配权衡。声明与真实使用不匹配可能降低性能或限制合法 Launch。

##### Q191：Loop Unrolling 为什么可能加速？

**答：** 展开减少循环控制并暴露 ILP、常量索引和向量化机会。过度展开会增大代码、寄存器压力和 Instruction Cache 压力。

##### Q192：Kernel Fusion 的收益是什么？

**答：** 减少 Kernel Launch，并让中间结果留在寄存器或 Shared Memory，避免 HBM 写回再读。对于 Memory-bound 的逐元素算子收益通常明显。

##### Q193：Kernel Fusion 为什么可能变慢？

**答：** 融合后寄存器和 Shared Memory 增加、Occupancy 降低，不同阶段的最佳并行粒度也可能冲突。过大 Kernel 还会增加编译和调度复杂度。

##### Q194：Vectorization 的核心收益是什么？

**答：** 主要是减少内存指令和地址计算数量，并匹配硬件宽事务；它不会减少必要数据字节。必须满足对齐、边界和寄存器容量。

##### Q195：ILP 是什么？

**答：** Instruction-level Parallelism 指单线程/Warp 中存在多个彼此独立的指令，使硬件可覆盖依赖延迟。较低 Occupancy 也可能通过高 ILP 达到好性能。

##### Q196：TLP 和 ILP 的区别是什么？

**答：** TLP 通过更多 Warp/Thread 隐藏延迟，ILP 通过同一执行上下文中的独立操作隐藏延迟。两者都会消耗寄存器和调度资源，需要平衡。

##### Q197：Nsight Systems 和 Nsight Compute 的区别是什么？

**答：** NSYS 看全程序时间线、CPU/GPU 交互、Stream、Memcpy 和通信；NCU 深入单个 Kernel 的指令、吞吐、Cache、Occupancy 与 Stall。通常先 NSYS 找热点，再 NCU 分析原因。

##### Q198：NCU 的 Warp Stall 指标怎么理解？

**答：** Stall Reason 表示 Scheduler 没有从该 Warp 发射下一指令的原因，如等待数据、Barrier 或执行管线。它是诊断线索，不能看到某项最高就直接套优化。

##### Q199：`long scoreboard` 常意味着什么？

**答：** 通常表示等待 Global/Local 等长延迟内存依赖，但还需结合 Memory Throughput、Cache 命中和访问模式确认。提高并行度、预取或减少依赖链可能有效。

##### Q200：`short scoreboard` 常意味着什么？

**答：** 常与 Shared Memory、MIO 或较短延迟依赖相关，Bank Conflict 和异步流水同步可能是原因。应查看 Shared Wavefront 与相关 Pipeline 指标。

##### Q201：Barrier Stall 高说明什么？

**答：** Warp 经常等待 CTA/Cluster 同步，可能来自工作不均、Barrier 过多或 Producer-Consumer 失衡。删除 Barrier 前必须先保证内存正确性。

##### Q202：DRAM Throughput 低说明不是 Memory-bound 吗？

**答：** 不一定。Kernel 可能是 Memory-latency-bound、受 L2/L1/Shared 带宽限制，或非合并请求导致指令等待但总吞吐不高。

##### Q203：Tensor Core Utilization 低可能有哪些原因？

**答：** Shape 太小、Tile 不匹配、数据搬运跟不上、K-loop 太短、Pipeline 有空洞或未真正生成 Tensor Core 指令。需要同时检查 SASS 与 Feed Pipeline。

##### Q204：如何正确做 CUDA Microbenchmark？

**答：** 固定输入与环境、Warmup、重复多次，用 CUDA Event 测目标 Stream，避免把初始化/JIT 混入，并验证结果正确。必要时锁定时钟并报告 GPU、CUDA 和数据 Shape。

##### Q205：为什么只测一次不可靠？

**答：** 首次运行可能含 JIT、Cache Cold Start 和频率爬升，系统还存在温度与并发噪声。应报告中位数或稳定分位数，而不是挑最快一次。

##### Q206：为什么优化后要重新验证数值？

**答：** 并行 Reduction 顺序、Fast Math、低精度和融合会改变舍入误差，竞态也可能只在特定输入出现。性能结果必须建立在满足误差标准的正确实现上。

##### Q207：`--use_fast_math` 做了什么？

**答：** 它启用更快但精度和 IEEE 语义可能较弱的运算，如近似特殊函数、FTZ 和较激进的除法。是否可用取决于误差容忍，不是无条件优化。

##### Q208：如何查看寄存器和 Shared Memory 使用量？

**答：** 编译时用 `-Xptxas -v` 查看寄存器、Shared、Stack/Spill；运行时用 NCU 或 Runtime Function Attributes。模板不同实例可能使用量完全不同。

##### Q209：如何查看最终指令？

**答：** 使用 `cuobjdump` 或 `nvdisasm` 查看 Cubin SASS，确认是否生成向量 Load、Tensor Core、异步 Copy 等目标指令。源码或 PTX 只能说明意图。

##### Q210：优化 CUDA Kernel 的推荐顺序是什么？

**答：** 先验证正确性，再用 NSYS 找端到端热点，用 NCU 判断瓶颈，然后优先修复数量级问题，如非合并访问、重复 HBM、低并行度和同步空洞，最后才做微小指令优化。

#### 八、Reduction、Scan、Softmax 等手写题：Q211-Q235

##### Q211：并行 Reduction 的基本思路是什么？

**答：** 每个线程先处理一个或多个元素，再按树形结构逐步合并局部结果，最后每个 Block 输出一个 Partial。大数组通常再用第二个 Kernel 合并 Partial。

##### Q212：为什么最朴素的交错 Reduction 效率低？

**答：** `tid % (2*s) == 0` 会造成 Warp Divergence，活跃线程分散，并可能引入不友好的 Shared Memory 访问。顺序寻址的折半归约更规整。

##### Q213：为什么 Reduction 每轮都可能需要 `__syncthreads()`？

**答：** 下一轮读取的是上一轮由其他线程写入的 Shared Memory，必须保证写入完成且可见。进入单个 Warp 后可改用带 Mask 的 Shuffle，减少块级 Barrier。

##### Q214：Warp Reduction 如何用 Shuffle 实现？

**答：** 每轮用 `__shfl_down_sync(mask, value, offset)` 读取高 Lane 的值并相加，Offset 依次为 16、8、4、2、1。只有参与 Lane 有效，Mask 必须正确。

##### Q215：为什么 Grid 级 Reduction 常用两阶段 Kernel？

**答：** 普通 Grid 没有廉价全局 Barrier，第一阶段每 Block 输出 Partial，Kernel 边界提供全局同步，第二阶段再归约。它简单、可移植且避免高竞争原子。

##### Q216：什么时候全局 `atomicAdd` Reduction 反而合适？

**答：** Block 数较少、硬件原子吞吐高或结果维度较多使竞争分散时，一次 Block-level Atomic 可能比第二次 Launch 更便宜。应与两阶段方案实测。

##### Q217：并行浮点 Reduction 为什么与 CPU 结果不完全一致？

**答：** 浮点加法不满足结合律，并行树改变了求和顺序，舍入误差会不同。需要用误差容限、FP32 累加、Pairwise/Kahan 等方法控制误差。

##### Q218：Prefix Scan 和 Reduction 的区别是什么？

**答：** Reduction 只输出总聚合值，Scan 输出每个位置之前的前缀结果。Scan 需要 Up-sweep/Down-sweep 或 Warp Scan 等更复杂的数据传播。

##### Q219：Hillis-Steele 和 Blelloch Scan 有何区别？

**答：** Hillis-Steele 容易实现但工作量约 `O(n log n)`；Blelloch 是 Work-efficient 的 `O(n)`，通过 Up-sweep 建树、Down-sweep 传播前缀，但同步和索引更复杂。

##### Q220：如何做跨 Block Scan？

**答：** 第一阶段对每 Block 扫描并输出 Block Sum，第二阶段扫描这些 Sum，第三阶段把对应 Block Prefix 加回。也可用 Decoupled Look-back 等单 Pass 算法，但同步协议更复杂。

##### Q221：Histogram 的主要性能问题是什么？

**答：** 大量线程对少量 Bin 做 Atomic，竞争会高度集中。常见优化是每 Warp/Block 在 Shared Memory 建私有 Histogram，再合并到 Global。

##### Q222：Shared Histogram 一定更快吗？

**答：** 不一定。Bin 很多时 Shared Memory 占用大，数据分布均匀且全局原子吞吐足够时私有化收益可能有限；还要考虑最后合并成本。

##### Q223：数值稳定 Softmax 为什么先减最大值？

**答：** `exp(x)` 对大正数可能溢出，减去同一行最大值不改变 Softmax 比例，却把最大指数变成 1，使其他指数落在可表示范围。

##### Q224：Softmax 通常需要哪些 Reduction？

**答：** 先求行最大值，再求 `sum(exp(x-max))`，最后归一化。高性能实现会尽量把行数据和中间值留在寄存器/Shared Memory，减少 HBM 往返。

##### Q225：Online Softmax 是什么？

**答：** 它分块读取数据并维护运行最大值 `m` 与归一化和 `l`；遇到更大最大值时，用 `exp(m_old-m_new)` 修正旧和。这样无需先保存整行 Logit。

##### Q226：两个 Online Softmax 分块如何合并？

**答：** `m=max(m1,m2)`，`l=exp(m1-m)l1+exp(m2-m)l2`。Attention 还要用相同比例合并加权输出 `o`，最后计算 `o/l`。

##### Q227：LayerNorm 的均值方差怎样并行计算？

**答：** 可分别归约 `sum(x)` 和 `sum(x²)`，但数值稳定性一般；Welford 在线算法维护 Count、Mean 和 M2，更适合大范围或低精度输入。

##### Q228：RMSNorm 为什么通常比 LayerNorm 简单？

**答：** RMSNorm 只需要均方根，不需要减均值，因此少一次统计量和部分算术。两者仍需读取整行并做 Reduction。

##### Q229：矩阵转置手写题的核心优化点是什么？

**答：** Global Read/Write 都要合并，使用 Shared Tile 改变访问方向，并通过 Padding/Swizzle 避免 Shared Bank Conflict。边界 Tile 需要 Mask。

##### Q230：Stencil Kernel 为什么需要 Halo？

**答：** 输出 Tile 的边缘依赖相邻输入，加载 Shared Tile 时要额外加载周围 Halo。Halo 会造成 CTA 间重复读取，但换来 Tile 内邻域复用。

##### Q231：卷积为什么可转成 GEMM？

**答：** im2col 将每个滑动窗口展平为矩阵行，与卷积核矩阵相乘。它复用成熟 GEMM，但显式 im2col 会膨胀内存，因此高性能库通常隐式生成 Tile。

##### Q232：如何手写 Vector Add？

**答：** 每线程处理一个或 Grid-stride Loop 中多个元素，保证相邻线程访问相邻地址并做边界检查。这个题主要考索引、Launch 配置和错误检查。

##### Q233：什么是 Grid-stride Loop？

**答：** 线程从全局 ID 开始，每次增加 `blockDim.x * gridDim.x` 处理后续元素。它允许用固定 Grid 覆盖任意长度，也适合 Persistent 风格与调试。

##### Q234：Grid-stride Loop 会破坏合并访问吗？

**答：** 通常不会，因为同一轮相邻 Lane 仍访问相邻元素；下一轮整体平移一个 Grid Stride。关键是 Stride 公式和元素布局正确。

##### Q235：手写 Kernel 面试时先保证什么？

**答：** 先说明输入 Shape、Layout、数据类型、边界和正确性，再给并行映射，最后讨论合并访问、复用、同步与资源。直接写高度优化代码却不解释约束通常风险更大。

#### 九、GEMM 与 Tensor Core：Q236-Q260

##### Q236：朴素 GEMM 为什么慢？

**答：** 每个输出元素独立从 Global Memory 重复读取整行 A 和整列 B，数据复用差，B 的列访问还可能不合并。大部分时间耗在冗余访存。

##### Q237：Tiled GEMM 如何减少 Global Memory 访问？

**答：** CTA 把 A/B 子块协作加载到 Shared Memory，每个元素在 Tile 内被多个线程重复用于 FMA。Global 读取次数相对朴素实现显著减少。

##### Q238：GEMM 的 CTA Tile、Warp Tile 和 Thread Tile 分别是什么？

**答：** CTA Tile 是一个 Block 负责的输出区域，继续划分给多个 Warp，Warp Tile 再划分到 Lane/Thread 持有的寄存器 Accumulator。分层 Tiling 对应 GPU 调度和存储层级。

##### Q239：Register Blocking 有什么作用？

**答：** 每个线程在寄存器中累计多个输出元素，使加载的一小段 A/B 被多次复用，提高 ILP 和 Arithmetic Intensity。Thread Tile 太大则增加寄存器压力。

##### Q240：GEMM 的 K-loop 为什么适合 Pipeline？

**答：** 每个 K Tile 都重复“加载下一块、计算当前块”，依赖结构规则，可通过 Double/Multi-stage Buffer 重叠 TMA/Copy 与 MMA。

##### Q241：Tensor Core 计算什么？

**答：** Tensor Core 执行小矩阵乘加 `D=A×B+C`，吞吐远高于标量 Core，但对数据类型、布局、对齐和 Tile Shape 有要求。

##### Q242：WMMA API 是什么？

**答：** `nvcuda::wmma` 是较高层的 Warp Matrix API，用 Fragment 表示输入和 Accumulator，便于使用 Tensor Core。它支持范围有限，高性能库更多使用 CUTLASS/CuTe 或底层 MMA 指令。

##### Q243：`mma.sync` 是什么？

**答：** 它是 Ampere 等架构常用的 Warp-level Tensor Core PTX 指令，一整个 Warp 协同提供 Fragment 并同步得到 Accumulator。

##### Q244：WGMMA 是什么？

**答：** Hopper 的 Warpgroup MMA 由 4 个 Warp，也就是 128 线程协作发起异步矩阵乘，输入主要来自 Shared Memory，适合与 TMA 和 Warp Specialization 组成流水。

##### Q245：UMMA/`tcgen05.mma` 是什么？

**答：** Blackwell 第五代 Tensor Core 指令可由单线程发起，Accumulator 位于 TMEM，并支持 1-CTA 或 2-CTA 协作及 Block-scaled 低精度类型。

##### Q246：为什么 Tensor Core 常使用低精度输入和 FP32 累加？

**答：** 低精度减少存储与带宽并提高乘法吞吐，较高精度 Accumulator 降低长 K Reduction 的误差。输出最终可再转换为 BF16/FP16/FP8。

##### Q247：TF32 是什么？

**答：** TF32 使用 FP32 的指数范围和较短尾数，在 Ampere Tensor Core 上加速部分 FP32 GEMM。它不是独立的 C++ 存储类型，是否使用由库和 Math Mode 控制。

##### Q248：FP8 的 E4M3 和 E5M2 有何差异？

**答：** E4M3 尾数更多、精度较好但范围较小；E5M2 指数更多、范围更大但精度更低。训练中常按前向、反向张量动态范围选择并配合 Scale。

##### Q249：Block Scaling 是什么？

**答：** 将张量分块，每块共享一个 Scale，使低位数据只表示局部动态范围。Scale 粒度越细通常精度越好，但 Metadata、Layout 和 Kernel 成本越高。

##### Q250：为什么 Tensor Core Kernel 对 Layout 很敏感？

**答：** MMA 指令要求 Lane 以特定方式提供 Fragment，Shared Memory 还要满足对齐、Swizzle 和 Bank 访问。逻辑 Row-major 相同，不代表物理布局都能高效喂给 Tensor Core。

##### Q251：什么是 GEMM Epilogue？

**答：** MMA 完成后将 Accumulator 变换并写回的阶段，可融合 Alpha/Beta、Bias、Activation、Scale、Residual 和类型转换，避免额外读写输出矩阵。

##### Q252：Split-K GEMM 是什么？

**答：** 多个 CTA 分别计算 K 维子区间的 Partial `C`，再做归并。它为小 M/N、大 K 增加并行度，但需要 Workspace、原子或串行归并。

##### Q253：Serial Split-K 和 Parallel Split-K 有何区别？

**答：** Serial Split-K 通过 Semaphore 让多个 Slice 依次累加同一输出，省 Workspace 但有顺序等待；Parallel Split-K 把 Partial 写到 Workspace 后单独归约，并行度更高但流量更大。

##### Q254：Stream-K 是什么？

**答：** Stream-K 把 GEMM 的 K 迭代工作更均匀地分给固定 CTA，而不是让每 CTA 只负责完整输出 Tile，用于减少最后一波负载不均。部分 Tile 需要跨 CTA 归并。

##### Q255：Grouped GEMM 是什么？

**答：** 一次 Kernel 处理多个独立、Shape 可不同的 GEMM，常用于 MoE Expert 和小矩阵集合。核心难点是 Problem Metadata、Tile 调度和不规则负载均衡。

##### Q256：Grouped GEMM 和 Batched GEMM 有何区别？

**答：** Batched GEMM 通常要求批内 Shape 相同，可通过固定 Stride 定位；Grouped GEMM 允许每组 Shape、Pointer 和 Leading Dimension 不同，调度成本更高但无需 Padding。

##### Q257：为什么 MoE 适合 Grouped GEMM？

**答：** 每个 Expert 的 `N/K` 通常相同，但收到的 Token 数 `M_e` 不同。Grouped GEMM 可把这些变长小 GEMM 放进一个任务池，减少 Launch 和 Padding。

##### Q258：cuBLAS 和自定义 GEMM Kernel 如何选择？

**答：** 首先把 cuBLAS/cuBLASLt 作为正确性和性能基线；只有特殊 Shape、融合、Layout 或动态调度使通用库不理想时才维护自定义 Kernel。

##### Q259：为什么手写 GEMM 很难超过 cuBLAS？

**答：** cuBLAS 有大量架构专用 Kernel、运行时选核、特殊 Shape 策略和长期调优。手写实现只有在范围更窄、融合更多或掌握额外业务约束时更可能胜出。

##### Q260：比较 GEMM 性能时必须报告什么？

**答：** 至少报告 GPU、CUDA/库版本、M/N/K、Batch、Layout、Dtype、累加精度、Epilogue、Warmup 和计时方法，并验证数值误差；只给 TFLOPS 没有可比性。

#### 十、Ampere、Hopper 与 Blackwell：Q261-Q280

##### Q261：Ampere 对 Kernel 开发最重要的变化是什么？

**答：** 代表性变化是 `cp.async` 支持 Global-to-Shared 异步流水，以及更成熟的 Warp-level `mma.sync` Tensor Core 路径。高性能 Kernel 开始更系统地重叠搬运和计算。

##### Q262：Ampere 的 `cp.async` 为什么能减少寄存器压力？

**答：** 传统路径常先 Global Load 到寄存器再 Store 到 Shared，`cp.async` 可直接完成 Global-to-Shared 复制，减少中转寄存器和相关指令。

##### Q263：Ampere GEMM 的主要执行主体是什么？

**答：** 通常以 Warp 为 MMA 协作主体，Warp 内执行 `mma.sync`，多个 Warp 在 CTA 内协作搬运和计算。Accumulator 主要保存在普通寄存器中。

##### Q264：Hopper 对 Kernel 开发最重要的变化是什么？

**答：** Hopper 引入 TMA、WGMMA、Thread Block Cluster 和 Distributed Shared Memory，使执行模型从 Warp-centric 向 Warpgroup 与 Producer-Consumer Warp Specialization 演进。

##### Q265：Hopper TMA 能搬运什么？

**答：** TMA 可依据 Tensor Map 在 Global 与 Shared 之间搬运 1D 到多维 Tensor，并支持边界处理、部分变换、Cluster Multicast 和异步完成通知。

##### Q266：WGMMA 为什么以 Warpgroup 为单位？

**答：** 更大的 128 线程协作范围能驱动更大 Tensor Core Tile，并与 TMA 大块供数匹配。代价是寄存器、Shared Memory 和 Barrier 设计更加整体化。

##### Q267：Hopper Thread Block Cluster 解决什么问题？

**答：** 它让一组 CTA 保证协同调度，可进行 Cluster Barrier、访问 Distributed Shared Memory，并通过 TMA Multicast 共享数据，扩展了单 CTA 的协作范围。

##### Q268：Distributed Shared Memory 比本 CTA Shared Memory 更快吗？

**答：** 通常本地 Shared Memory 更低延迟；DSMEM 的价值是让 Cluster 内 CTA 直接共享片上数据，避免绕行 Global Memory。应只在跨 CTA 复用能覆盖远端成本时使用。

##### Q269：Hopper Warp Specialization 是什么？

**答：** 不同 Warp 长期承担 TMA Load、WGMMA Compute 或 Epilogue 等角色，通过多 Stage Barrier 流水协作。它让搬运与计算重叠，并可把更多寄存器分给 Consumer Warpgroup。

##### Q270：Hopper 为什么广泛使用 FP8？

**答：** Hopper Tensor Core 原生支持 FP8，可降低权重/激活带宽并提高矩阵乘吞吐。E4M3/E5M2 仍需配合 Scale、Amax 和高精度累加维持数值稳定。

##### Q271：Blackwell 对 Kernel 开发最重要的变化是什么？

**答：** SM100 引入 `tcgen05/UMMA`、TMEM、1SM/2SM MMA、硬件 Block Scale 和更强的 Persistent 调度能力，使 MMA Issue、Accumulator 和 Epilogue 可以进一步解耦。

##### Q272：TMEM 是什么？

**答：** Tensor Memory 是 Blackwell SM 内专门服务 Tensor Core 的片上二维存储，Accumulator 必须位于 TMEM，部分场景也可放 Operand A。它减少普通 Register File 的 Accumulator 压力。

##### Q273：`tcgen05.mma` 为什么可以单线程发起？

**答：** 指令通过 Descriptor 描述 Shared/TMEM Operand，由一个选举线程提交异步 Tensor Core 工作；数据协作不再要求所有计算线程共同发出同一 MMA 指令。

##### Q274：Blackwell 1-CTA 和 2-CTA MMA 有何区别？

**答：** 1-CTA 由一个 CTA/SM 执行，2-CTA 由 Cluster 中相邻 CTA/SM 共同执行更大 MMA Tile。2-CTA 提高复用和吞吐机会，但要求 Cluster、TMEM 和 Tile 对齐。

##### Q275：Cluster Launch Control 是什么？

**答：** CLC 允许正在运行的 CTA/Cluster 取消尚未开始的工作并接管其任务，用于 Persistent Kernel 的硬件辅助 Work Stealing，减少静态 Tile 分配造成的尾部不均。

##### Q276：NVFP4 的优化重点是什么？

**答：** NVFP4 使用 4-bit 数据和细粒度 Scale，显著降低权重带宽并提高 Tensor Core 吞吐；Kernel 必须处理 Scale Layout、Block-scaled MMA 和高精度累加，不能当普通 INT4。

##### Q277：数据中心 Blackwell SM100 和消费级 SM120 能共用 Kernel 吗？

**答：** 不能默认共用。两者 Compute Capability、Tensor Core 指令、TMEM/TMA 能力和资源不同，应分别编译和分派，SM100 的 `tcgen05` Kernel 不能直接假设可在 SM120 运行。

##### Q278：为什么有 `sm_90a`、`sm_100a` 这类架构专用目标？

**答：** 后缀 `a` 表示包含不能向同主版本其他实现保证兼容的架构特性，例如 WGMMA 或特定 `tcgen05` 能力。使用这些指令的 Cubin 必须针对对应 Architecture-specific Target 编译。

##### Q279：PTX 能让旧 Kernel 自动利用所有新硬件吗？

**答：** 不能。PTX JIT 可提供功能兼容和部分重新优化，但旧 PTX 没有表达 TMA、WGMMA、TMEM 等新算法结构，想充分利用新架构通常需要重写数据流。

##### Q280：从 Ampere Kernel 迁移到 Hopper/Blackwell 应先改什么？

**答：** 先重新设计搬运、MMA 和 Accumulator 数据流，而不是只替换指令名；再确定 Warp/CTA 角色、Pipeline Stage、Cluster 和 Epilogue，最后按目标硬件重新调 Tile。

#### 十一、大模型算子与灵活推理题：Q281-Q300

##### Q281：LLM Prefill 和 Decode 的算子特征有何不同？

**答：** Prefill 一次处理很多 Query Token，GEMM 和 Attention 计算量大；Decode 每步通常只有一个新 Token，读取大量历史 KV，常受显存带宽和 Launch 延迟限制。

##### Q282：标准 Attention 为什么会产生 `O(L²)` 中间量？

**答：** `Q[L,D]K^T[D,L]` 形成 `L×L` Score Matrix，再做 Softmax 和乘 V。直接物化该矩阵会产生巨大 HBM 流量和容量需求。

##### Q283：FlashAttention 的核心是什么？

**答：** 将 Q/K/V 分块留在片上，使用 Online Softmax 流式更新最大值、归一化和与输出，避免把完整 Attention Matrix 写入 HBM。它减少 IO，不是近似 Attention。

##### Q284：FlashAttention 为什么仍要多次读取 K/V Tile？

**答：** 片上空间放不下全部序列，不同 Q Tile 会依次遍历所需 K/V Tile。优化目标是在容量约束下最大化每次读取的复用，而不是让每个 K/V 只读一次。

##### Q285：KV Cache 保存什么？

**答：** 自回归 Decode 保存历史 Token 在每层 Attention 中的 K 和 V，使下一步无需重新计算整个前缀。其容量与层数、上下文、KV Head 数、Head Dim 和数据类型成正比。

##### Q286：Paged KV Cache 解决什么问题？

**答：** 将逻辑连续 KV 切成固定大小 Page，通过 Block Table 映射到非连续物理块，减少动态请求长度导致的外部碎片，并支持共享前缀和按需扩容。

##### Q287：MQA/GQA 为什么能减少 KV Cache？

**答：** MQA 让所有 Query Head 共享一组 KV Head，GQA 让每组 Query Head 共享 KV；KV Head 数减少后 Cache 和 Decode 读取量按比例下降。

##### Q288：Split-KV 是什么？

**答：** 将长 KV 序列切成多个区间，由多个 CTA 并行计算局部 Online Softmax 状态 `(m_s,l_s,o_s)`，第二阶段数值稳定地归并。它为小 Batch、单 Token Decode 增加并行度。

##### Q289：为什么 Decode Attention 常是 Memory-bound？

**答：** 一个 Query 只对每个历史 K/V 做少量点积，却必须读取整个 Cache，数据复用低，Arithmetic Intensity 小。上下文越长，KV 读取越容易主导时间。

##### Q290：CUDA Graph 为什么能降低 Decode 延迟？

**答：** Decode 每步拓扑重复且包含许多短 Kernel，Graph 一次提交整步依赖，减少 CPU Launch Gap。它不能消除 KV Cache 的 HBM 带宽成本。

##### Q291：为什么融合 QKV Preprocess、RoPE 和 Cache Write？

**答：** 这些操作依次处理同一批 Q/K/V 元素，分开会重复读写 HBM并增加 Launch。融合后可在寄存器中完成位置旋转和 Layout 转换，再直接写 Cache。

##### Q292：KV Cache 量化的收益和代价是什么？

**答：** FP8/INT8/INT4 Cache 减少容量与 Decode 读取字节，但需要 Scale Metadata、反量化和专用 Attention Kernel，并可能损失精度。收益通常在长上下文更明显。

##### Q293：MoE Router 后为什么要 Permute Token？

**答：** 同一 Expert 的 Token 原本分散，把它们按 Expert 排成连续段后，才能形成规则 GEMM 并进行合并访存。输出还需按原 Token 与 Top-K Slot 做 Unpermute/Combine。

##### Q294：MoE 的 Grouped GEMM 为什么仍可能很慢？

**答：** 每 Expert Token 数可能很小且极不均衡，导致窄 GEMM、尾部浪费、权重读取和 Expert Parallel All-to-All 成为瓶颈。单次 Launch 只解决一部分问题。

##### Q295：Persistent CTA 为什么适合 Attention 和 MoE？

**答：** 它把不同请求、Head、KV Split 或 Expert Tile 放入统一任务池，完成短任务的 CTA 可继续领取工作，减少多次小 Kernel 的 SM 空闲与不规则长尾。

##### Q296：拿到一个很慢的未知 Kernel，如何分析？

**答：** 先确认结果与 Shape，NSYS 判断是 Launch、Copy 还是 Kernel，NCU 再判断 Compute、Bandwidth、Latency、同步和资源限制；最后针对最大瓶颈提出可验证的单变量改动。

##### Q297：Kernel 结果偶尔错误，优先怀疑什么？

**答：** 优先检查越界、未初始化、缺 Barrier、错误 Stream 依赖、Data Race 和异步生命周期，再检查低精度误差。使用 Compute Sanitizer，并把错误检查放到正确同步点。

##### Q298：CUDA P2P 和 NCCL 有何区别？

**答：** CUDA P2P 提供 GPU 间直接内存访问与 Copy 的底层能力；NCCL 在其上结合 NVLink、PCIe 和网络实现 AllReduce、AllGather、All-to-All 等集体通信并进行拓扑优化。

##### Q299：GPUDirect RDMA 解决什么问题？

**答：** 它允许支持的网卡直接 DMA 访问 GPU Memory，避免网络数据经 CPU Buffer 额外复制。端到端性能仍依赖 PCIe/NVLink 拓扑、注册内存、消息大小和通信协议。

##### Q300：面试中回答 CUDA 优化题的通用框架是什么？

**答：** 先定义 Shape、Layout、Dtype 和正确性，再说明线程/Tile 映射，计算 FLOPs 与必要字节判断瓶颈，分析合并访问、复用、同步和 Occupancy，最后给出基线、Profile 指标及优化取舍。

#### 十二、硬件数值与资源计算：Q301-Q340

这一节的 `KB/K` 按 NVIDIA 官方表格表示 1024。除非问题明确指定 A100、H100 或 B200，否则应先说明数值依赖 Compute Capability。

##### Q301：当前 NVIDIA CUDA Warp 有多少线程？

**答：** 32 个。`warpSize` 在当前主流 Compute Capability 中为 32，写代码时仍建议使用内建变量而不是把 32 散落在索引公式中。

##### Q302：一个 Thread Block 最多有多少线程？

**答：** 当前表中上限为 1024。三维 Block 的 `x*y*z` 乘积不能超过 1024，不是每个维度都取到最大后还能同时成立。

##### Q303：Thread Block 三个维度的上限是多少？

**答：** 当前表中 `x<=1024`、`y<=1024`、`z<=64`，同时总线程数不超过 1024。例如 `(32,32,1)` 合法，`(1024,1024,1)` 不合法。

##### Q304：Grid 三个维度的上限是多少？

**答：** 当前表中 `grid.x<=2^31-1`，`grid.y/grid.z<=65535`。实际程序仍受任务规模、Launch 参数类型和总执行时间约束。

##### Q305：一个 Device 最多可有多少个 Resident Grid？

**答：** 当前 Compute Capability 表给出的 Concurrent Resident Grid 上限为 128。它只是 Grid 数量上限，实际并发还受每个 Kernel 的 SM 资源占用限制。

##### Q306：A100、H100 和 B200 每个 SM 最多驻留多少个 CTA？

**答：** 对应 SM80、SM90、SM100 的架构上限都是 32 个 Resident CTA/SM。只有 CTA 极小且其他资源足够时才能接近该数值。

##### Q307：A100、H100 和 B200 每个 SM 最多驻留多少个 Warp？

**答：** 都是 64 个 Warp/SM。由于一个 Warp 32 线程，这与 2048 个 Resident Thread/SM 的线程上限一致。

##### Q308：A100、H100 和 B200 每个 SM 最多驻留多少个线程？

**答：** 都是 2048 个 Resident Thread/SM。它是架构上限，寄存器、Shared Memory 和 CTA 数限制可能使实际线程数更少。

##### Q309：A100、H100 和 B200 每个 SM 有多少寄存器？

**答：** 都是 64K 个 32-bit Register，也就是 65,536 个 32-bit 槽位。这里的 K 表示 1024，不是 64,000。

##### Q310：一个 CTA 最多能分配多少个 32-bit Register？

**答：** 当前表中是 64K 个，与每 SM Register File 总槽位相同。通常还会先受到每线程 255 个寄存器或分配粒度限制。

##### Q311：一个线程最多能使用多少个 Register？

**答：** 当前表中最多 255 个 32-bit Register。接近该上限通常会严重压缩 Occupancy，并可能因编译器决策产生 Spill。

##### Q312：一个 64-bit 变量占几个 Register？

**答：** 寄存器统计以 32-bit 槽位计，一个 64-bit 标量通常占两个槽位；更宽向量按组成元素继续展开。最终数量以 `ptxas -v` 为准。

##### Q313：256 线程、每线程 64 个寄存器，寄存器最多允许几个 CTA 驻留？

**答：** 每 CTA 理论使用 `256*64=16,384` 个槽位，`65,536/16,384=4`，所以寄存器维度最多允许 4 个 CTA/SM。实际还要取其他资源限制的最小值。

##### Q314：256 线程、每线程 96 个寄存器，寄存器最多允许几个 CTA 驻留？

**答：** 每 CTA 需要 `256*96=24,576` 个槽位，`floor(65,536/24,576)=2`，第三个 CTA 放不下；实际分配粒度还可能进一步收紧。

##### Q315：A100 每个 SM 有多少 Shared Memory？

**答：** SM80 A100 最多 164 KiB Shared Memory/SM，可通过 Carveout 在多个容量档位间配置。它与统一 L1/Shared 资源相关。

##### Q316：A100 单个 CTA 最多可寻址多少 Shared Memory？

**答：** 163 KiB。SM 有 164 KiB，但 CUDA 为每个 CTA 保留 1 KiB，因此单 CTA 上限少 1 KiB。

##### Q317：H100 与 B200 每个 SM 有多少 Shared Memory？

**答：** SM90 H100 和 SM100 B200 都最多为 228 KiB Shared Memory/SM。两者还支持 0、8、16、32、64、100、132、164、196、228 KiB 等 Carveout 档位。

##### Q318：H100 与 B200 单个 CTA 最多可寻址多少 Shared Memory？

**答：** 227 KiB。原因同样是每 CTA 保留 1 KiB；超过默认阈值还需要显式 Opt-in。

##### Q319：为什么静态 Shared Memory 常看到 48 KiB 限制？

**答：** 为保持架构兼容，静态 Shared Allocation 仍限制在 48 KiB；超过 48 KiB 通常要使用 Dynamic Shared Memory，并通过 `cudaFuncSetAttribute` 显式允许更大的每 CTA Dynamic Shared 上限。

##### Q320：SM86 与 A100 的资源数字相同吗？

**答：** 不同。SM86 最大 48 Warp、1536 Thread、16 CTA/SM，Shared Memory 为 100 KiB/SM、99 KiB/CTA；虽然 Register File 同样是 64K，Occupancy 上限不同。

##### Q321：Constant Memory 总容量多大？

**答：** CUDA 对静态 Constant Memory 给出的容量是 64 KiB，每 SM 的 Constant Cache Working Set 为 8 KiB。容量 64 KiB 不代表所有数据都能同时命中 Cache。

##### Q322：每线程 Local Memory 的架构上限是多少？

**答：** 当前表中是 512 KiB/Thread。它是地址空间/资源上限，不是性能目标；Local Memory 位于 Device Memory，使用过多通常非常慢。

##### Q323：Shared Memory 有多少个 Bank？

**答：** 当前表中为 32 个 Bank。对常见 32-bit 访问，连续 32-bit Word 依次映射到 32 个 Bank。

##### Q324：最坏可能出现多少路 Shared Memory Bank Conflict？

**答：** 一个 32-Lane Warp 在访问同一 Bank 的 32 个不同地址时，可形成 32-way Conflict。若读取同一个地址则通常走 Broadcast，不按 32-way Conflict 处理。

##### Q325：一个 Warp 连续读取 32 个 `float`，请求多少有效字节？

**答：** `32*4=128` 字节。按现代 GPU 常用的 32-byte Sector 观察，若起始地址对齐且不跨额外边界，最少覆盖 4 个 Sector。

##### Q326：一个 Warp 连续读取 32 个 `half`，请求多少有效字节？

**答：** `32*2=64` 字节，对齐时最少覆盖 2 个 32-byte Sector。若地址错位跨边界，Sector 数会增加。

##### Q327：一个 Warp 每个 Lane 读取一个 `float4`，请求多少有效字节？

**答：** 每 Lane 16 字节，总计 `32*16=512` 字节，对齐时对应 16 个 32-byte Sector。向量化减少指令数，不会减少这 512 字节数据本身。

##### Q328：256 线程的 CTA 包含多少 Warp？

**答：** `256/32=8` 个 Warp。若目标 SM 最多 64 Warp，仅从 Warp 数看最多可驻留 8 个这种 CTA。

##### Q329：1024 线程的 CTA 包含多少 Warp？线程上限允许每 SM 放几个？

**答：** 包含 `1024/32=32` 个 Warp；在 2048 Thread/SM 的 SM80/90/100 上，仅从线程数看最多 2 个 CTA，寄存器和 Shared Memory 还可能把它降到 1。

##### Q330：H100 上每 CTA 使用 64 KiB Shared Memory，Shared 维度最多放几个 CTA？

**答：** `floor(228/64)=3`，因为 3 个使用 192 KiB，4 个需要 256 KiB。实际还要检查寄存器、线程和分配粒度。

##### Q331：H100 上每 CTA 使用 120 KiB Shared Memory，Shared 维度最多放几个 CTA？

**答：** `floor(228/120)=1`。即使线程和寄存器很少，也无法同时驻留第二个 120 KiB CTA。

##### Q332：256 线程、每线程 64 寄存器时，仅按寄存器计算的理论 Occupancy 是多少？

**答：** 最多 4 个 CTA，每 CTA 8 Warp，共 32 Warp；相对 64 Warp 上限是 `32/64=50%`。这是寄存器单项结果，不是最终 Occupancy。

##### Q333：H100 上 256 线程、每 CTA 96 KiB Shared Memory时，仅按 Shared 计算的 Occupancy 是多少？

**答：** 最多 2 个 CTA，共 `2*8=16` 个 Warp，因此相对 64 Warp 上限为 25%。若寄存器只允许 1 个 CTA，最终还会更低。

##### Q334：为什么 32 线程小 CTA 在 A100 上最多只有 50% Warp Occupancy？

**答：** 每 CTA 只有 1 个 Warp，而 A100 最多 32 CTA/SM，因此最多 32 Warp；相对 64 Warp 上限正好 50%，此时限制项是 CTA 数而不是线程数。

##### Q335：64 线程 CTA 在 A100 上理论上能达到多少 Warp Occupancy？

**答：** 每 CTA 2 Warp，最多 32 CTA 可形成 64 Warp，也就是 100%；前提是寄存器和 Shared Memory 足够。

##### Q336：100 万个元素、Block Size 256，需要启动多少个 CTA？

**答：** `ceil(1,000,000/256)=3907` 个 CTA，因为 `(1,000,000+255)/256=3907`。最后一个 CTA 只有部分线程有效，需要边界判断。

##### Q337：一个 `4096×4096×4096` GEMM 有多少 FLOPs？

**答：** 按一次乘加 2 FLOPs，计算量是 `2*4096^3=137,438,953,472` FLOPs，约 137.4 GFLOPs。用它除以秒数即可得到有效 FLOP/s。

##### Q338：32 层、8 个 KV Head、Head Dim 128、BF16、8K Context 的 KV Cache 多大？

**答：** 每层每 Token 为 `2(K/V)*8*128*2=4096` 字节；32 层是 128 KiB/Token；乘 8192 Token 得 1 GiB，尚未包含对齐、Page Metadata 和 Batch。

##### Q339：Blackwell SM100 的 TMEM 逻辑容量是多少？

**答：** 每个 CTA 可见的 TMEM 地址空间为 128 行、512 列、每格 32 bit，即 `128*512*4=262,144` 字节，也就是 256 KiB。实际物理分配按列进行，并受布局、分配粒度和 CTA Group 规则约束。

##### Q340：Thread Block Cluster 最大包含多少 CTA？

**答：** 可移植 Cluster Size 最大为 8；B200 可通过 `cudaFuncAttributeNonPortableClusterSizeAllowed` Opt-in 到 16。更大 Cluster 会减少可同时激活的 Cluster 数。

#### 十三、参考资料

1. [NVIDIA CUDA Programming Guide](https://docs.nvidia.com/cuda/cuda-programming-guide/)

1. [CUDA SIMT Kernel 与线程、内存基础](https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/writing-cuda-kernels.html)

1. [CUDA Advanced Kernel Programming](https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/advanced-kernel-programming.html)

1. [CUDA C++ Best Practices Guide](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)

1. [CUDA C++ Memory Model](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cuda-cpp-memory-model.html)

1. [CUDA Cooperative Groups](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/cooperative-groups.html)

1. [CUDA Graphs](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/cuda-graphs.html)

1. [PTX ISA](https://docs.nvidia.com/cuda/parallel-thread-execution/)

1. [CUDA Runtime API](https://docs.nvidia.com/cuda/cuda-runtime-api/)

1. [Compute Sanitizer](https://docs.nvidia.com/compute-sanitizer/ComputeSanitizer/)

1. [Nsight Systems User Guide](https://docs.nvidia.com/nsight-systems/UserGuide/)

1. [Nsight Compute Profiling Guide](https://docs.nvidia.com/nsight-compute/ProfilingGuide/)

1. [Ampere Tuning Guide](https://docs.nvidia.com/cuda/ampere-tuning-guide/)

1. [Hopper Tuning Guide](https://docs.nvidia.com/cuda/hopper-tuning-guide/)

1. [CUTLASS Blackwell SM100 GEMM](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/blackwell_functionality.html)

1. [CUTLASS `tcgen05` Programming Guide](https://docs.nvidia.com/cutlass/latest/media/docs/pythonDSL/mma_docs/tcgen05_programming.html)

1. [CUTLASS Documentation](https://docs.nvidia.com/cutlass/)

1. [CUDA Samples](https://github.com/NVIDIA/cuda-samples)

1. [NCCL User Guide](https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/)

1. [Compute Capability 资源上限表](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/compute-capabilities.html)

1. [Blackwell Tuning Guide](https://docs.nvidia.com/cuda/blackwell-tuning-guide/)

## 2 CUDA 基础与硬件模型

### CUDA 硬件架构：SM、Warp、CUDA Core 与 Tensor Core

> 原文：[CUDA 硬件架构：SM、Warp、CUDA Core 与 Tensor Core](https://hd9568.github.io/blog/cuda-hardware-sm-warp-cores/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**从 GPU 执行单元出发，理解 SM、Warp、CUDA Core、Tensor Core 的关系，以及它们为什么决定 CUDA kernel 的性能上限。

#### 一、核心结论

CUDA 程序的性能主要取决于大量线程如何被组织到 GPU 硬件上执行。

- SM（Streaming Multiprocessor）是 GPU 上主要的执行单元。

- 一个 GPU 有多个 SM，每个 SM 可以同时驻留多个 Block 和多个 Warp。

- Warp 是线程调度的基本单位，NVIDIA GPU 上通常是 32 个线程一组。

- CUDA Core 更适合执行标量 FP32/INT 等通用计算。

- Tensor Core 是矩阵乘加专用单元，适合 GEMM、卷积、Transformer 等张量计算。

- CUDA 优化的核心是让 SM 忙起来，同时减少访存、分支和同步浪费。

#### 二、GPU 为什么适合并行计算

CPU 更擅长复杂控制流和低延迟任务，GPU 更擅长大规模数据并行。

```text
CPU: 少量强核心，复杂控制逻辑，大 Cache
GPU: 大量计算单元，高吞吐，适合相同操作处理大量数据
```

例如向量加法：

```cpp
// 每个元素的计算彼此独立，适合交给大量 GPU 线程并行处理。
c[i] = a[i] + b[i];
```

如果有一百万个元素，可以让大量线程分别处理不同下标。

#### 三、SM 是什么

SM 可以理解为 GPU 内部的“并行计算车间”。一个 GPU 通常包含多个 SM，每个 SM 内部包含：

- Warp Scheduler：选择下一个要执行的 Warp。

- Register File：给线程保存寄存器。

- Shared Memory / L1 Cache：Block 内共享和缓存数据。

- CUDA Core：执行普通算术指令。

- Tensor Core：执行矩阵乘加指令。

- Load/Store Unit：处理访存指令。

- Special Function Unit：处理 sin、cos、rsqrt 等特殊函数。

一个 kernel 启动后，Grid 中的 Block 会被分配到不同 SM 上执行。

```text
Grid
+---------+---------+---------+
| Block 0 | Block 1 | Block 2 |  -> 分配到不同 SM
+---------+---------+---------+
SM 0: Block 0, Block 3, ...
SM 1: Block 1, Block 4, ...
SM 2: Block 2, Block 5, ...
```

Block 一旦被分配到某个 SM，通常会在该 SM 上执行到结束，不会中途迁移到别的 SM。

#### 四、Warp 是什么

Warp 是 GPU 调度和执行线程的基本单位。NVIDIA GPU 上一个 Warp 通常包含 32 个线程。

```text
Block with 128 threads
+---------+---------+---------+---------+
| Warp 0  | Warp 1  | Warp 2  | Warp 3  |
| 32 thr  | 32 thr  | 32 thr  | 32 thr  |
+---------+---------+---------+---------+
```

虽然 CUDA 代码里写的是“每个线程执行一份代码”，但硬件调度时通常按 Warp 发射指令。一个 Warp 内的线程执行同一条指令，只是处理不同数据。

这也是 SIMT（Single Instruction, Multiple Threads）模型。

#### 五、CUDA Core 与 Tensor Core

##### CUDA Core

CUDA Core 执行通用标量计算，例如：

```cpp
float z = x * y + b;
```

这类操作适合普通 CUDA Core。

##### Tensor Core

Tensor Core 面向小矩阵乘加，例如：

```text
D = A * B + C
```

它不是处理单个标量乘法，而是一次处理一小块矩阵乘加。深度学习中的 GEMM、卷积、Attention 都可以转化成矩阵乘加，因此 Tensor Core 对 AI Infra 很关键。

Tensor Core 常和这些数据类型相关：

- FP16 / BF16。

- TF32。

- INT8 / FP8（取决于硬件代际）。

##### 两者差异

| 对比项 | CUDA Core | Tensor Core |
| --- | --- | --- |
| 计算形态 | 标量/向量指令 | 矩阵乘加指令 |
| 常见数据类型 | FP32、INT32 等 | FP16、BF16、TF32、INT8、FP8 等 |
| 典型场景 | 通用 kernel | GEMM、卷积、Attention |
| 性能特点 | 灵活 | 吞吐极高但要求数据布局和类型匹配 |

#### 六、从 kernel 到硬件执行

一个简单 kernel：

```cpp
__global__ void add_kernel(const float* a, const float* b, float* c, int n) {
    // 每个线程负责一个元素。
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // 边界检查避免越界访问。
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}
```

启动：

```cpp
int threads = 256;
int blocks = (n + threads - 1) / threads;
add_kernel<<<blocks, threads>>>(a, b, c, n);
```

执行关系：

1. `blocks` 个 Block 组成一个 Grid。

1. 每个 Block 被调度到某个 SM。

1. Block 内线程被划分成多个 Warp。

1. SM 的 Warp Scheduler 选择 Warp 发射指令。

1. CUDA Core 或 Tensor Core 执行对应计算。

#### 七、性能理解要点

##### 1. SM 利用率

如果 Block 太少，很多 SM 没活干，GPU 利用率低。

##### 2. Warp 执行效率

Warp 内线程如果分支严重，可能导致同一个 Warp 分多次执行不同路径。

##### 3. 访存效率

多数 CUDA kernel 不是算力不够，而是数据从 Global Memory 取不够快。访存合并、共享内存复用、减少重复读写非常重要。

##### 4. Tensor Core 不是自动使用

代码需要满足合适的数据类型、矩阵形状和库调用方式。例如 cuBLAS、CUTLASS、WMMA 才能更直接地用上 Tensor Core。

#### 八、面试回答模板

如果问题是“SM、Warp、CUDA Core、Tensor Core 的关系”，可以这样回答：

1. GPU 由多个 SM 组成，SM 是主要执行单元。

1. Kernel 的 Block 会被调度到 SM 上执行，Block 内线程按 Warp 组织。

1. Warp 是调度基本单位，NVIDIA GPU 通常 32 个线程一个 Warp。

1. CUDA Core 执行普通标量计算，Tensor Core 执行矩阵乘加，适合深度学习算子。

1. CUDA 优化要关注 SM 是否充分占用、Warp 是否高效、访存是否合并、Tensor Core 是否被有效利用。

#### 九、总结

CUDA 硬件架构可以按层次理解：

```text
GPU -> SM -> Warp -> Thread -> Instruction
```

SM 决定并行执行资源，Warp 决定调度粒度，CUDA Core 和 Tensor Core 决定计算方式。性能优化不是只看线程数量，而是让计算、访存和调度都尽量高效。

### CUDA 线程模型：Grid、Block、Thread 与 Global Thread ID

> 原文：[CUDA 线程模型：Grid、Block、Thread 与 Global Thread ID](https://hd9568.github.io/blog/cuda-thread-model-grid-block-thread/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**用一维、二维、三维映射理解 CUDA Grid、Block、Thread 的组织方式，并讲清 Global Thread ID 的计算方法。

#### 一、核心结论

CUDA 用层次化线程模型组织并行计算。

- 一个 kernel 启动一个 Grid。

- 一个 Grid 包含多个 Block。

- 一个 Block 包含多个 Thread。

- `blockIdx` 表示当前 Block 在 Grid 中的位置。

- `threadIdx` 表示当前 Thread 在 Block 中的位置。

- `blockDim` 表示每个 Block 的维度。

- `gridDim` 表示 Grid 的维度。

- Global Thread ID 用于把每个线程映射到全局数据下标。

#### 二、Grid、Block、Thread 的关系

CUDA 启动语法：

```cpp
kernel<<<gridDim, blockDim>>>(args);
```

例如：

```cpp
dim3 block(256);
dim3 grid((n + block.x - 1) / block.x);
add_kernel<<<grid, block>>>(a, b, c, n);
```

层次结构：

```text
Grid
+-----------------------------+
| Block 0 | Block 1 | Block 2 |
| 256 thr | 256 thr | 256 thr |
+-----------------------------+
```

Block 是资源分配单位，Thread 是最细粒度执行单元，Warp 是硬件调度单位。

#### 三、一维 Global Thread ID

一维数组最常见。

```cpp
__global__ void add_kernel(const float* a, const float* b, float* c, int n) {
    // blockIdx.x：当前 block 的编号。
    // blockDim.x：每个 block 有多少线程。
    // threadIdx.x：当前线程在 block 内的编号。
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // n 不一定刚好整除 blockDim.x，因此必须做边界检查。
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}
```

启动：

```cpp
int threads_per_block = 256;
int blocks = (n + threads_per_block - 1) / threads_per_block;
add_kernel<<<blocks, threads_per_block>>>(a, b, c, n);
```

向上取整公式：

```cpp
blocks = (n + threads - 1) / threads;
```

它保证即使 `n` 不是 `threads` 的整数倍，也能覆盖所有元素。

#### 四、二维映射

二维映射适合图像、矩阵、二维张量。

```cpp
__global__ void scale_2d(float* x, int width, int height) {
    // col 对应矩阵列，row 对应矩阵行。
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row < height && col < width) {
        int offset = row * width + col;  // 行主序展开。
        x[offset] *= 2.0f;
    }
}
```

启动：

```cpp
dim3 block(16, 16);
dim3 grid((width + block.x - 1) / block.x,
          (height + block.y - 1) / block.y);
scale_2d<<<grid, block>>>(x, width, height);
```

二维 Block 常见选择是 `16x16` 或 `32x8`。选择要结合访存模式、寄存器、共享内存和 Occupancy。

#### 五、三维映射

三维映射适合三维网格、体数据、`batch x row x col` 这类结构。

```cpp
__global__ void scale_3d(float* x, int width, int height, int depth) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int z = blockIdx.z * blockDim.z + threadIdx.z;
    if (z < depth && row < height && col < width) {
        // 三维数组按 z, row, col 展开成一维。
        int offset = (z * height + row) * width + col;
        x[offset] *= 2.0f;
    }
}
```

启动：

```cpp
dim3 block(8, 8, 4);
dim3 grid((width + block.x - 1) / block.x,
          (height + block.y - 1) / block.y,
          (depth + block.z - 1) / block.z);
```

三维映射不是必须的。很多时候也可以手动把一维 `idx` 解码成多维坐标。

#### 六、线程块大小如何选择

线程块大小影响：

- 一个 Block 内有多少 Warp。

- 每个 SM 能驻留多少 Block。

- 寄存器和共享内存占用。

- 访存合并和执行效率。

常见经验：

- 一维向量操作：`128`、`256`、`512` 线程常见。

- 二维矩阵操作：`16x16`、`32x8` 常见。

- Block 线程数通常选择 Warp 大小 32 的倍数。

示例：

```cpp
// 256 是 32 的 8 倍，一个 block 包含 8 个 warp。
int threads = 256;
```

最终选择应通过 profiling 验证，而不是只靠经验。

#### 七、常见错误

##### 1. 忘记边界检查

```cpp
int idx = blockIdx.x * blockDim.x + threadIdx.x;
c[idx] = a[idx] + b[idx];  // n 不能整除时可能越界
```

正确写法：

```cpp
if (idx < n) {
    c[idx] = a[idx] + b[idx];
}
```

##### 2. 二维下标展开写错

行主序矩阵 `[row][col]` 展开为：

```cpp
offset = row * width + col;
```

常见错误是把 `width` 和 `height` 混用。

##### 3. Block 大小不是 Warp 倍数

不是绝对错误，但通常会导致最后一个 Warp 有大量无效线程。除非有特殊原因，线程数一般选 32 的倍数。

#### 八、面试回答模板

如果问题是“CUDA 中如何计算 global thread id”，可以这样回答：

1. CUDA kernel 启动一个 Grid，Grid 里有多个 Block，Block 里有多个 Thread。

1. 一维场景下，全局线程编号是 `blockIdx.x * blockDim.x + threadIdx.x`。

1. 二维场景下，分别计算 `row` 和 `col`，再用 `row * width + col` 展开到一维地址。

1. Grid 大小通常用向上取整公式 `(n + blockDim - 1) / blockDim`。

1. 因为总线程数可能大于数据元素数，所以 kernel 内必须做边界检查。

#### 九、总结

CUDA 线程模型本质上是把大规模数据映射给大量线程：

```text
数据下标 <-> Global Thread ID
```

一维、二维、三维只是不同数据形状的映射方式。只要下标计算正确、边界检查完整、Block 大小合理，后续优化才有可靠基础。

### CUDA Warp 机制：分支分化、Active Mask 与 Warp 级原语

> 原文：[CUDA Warp 机制：分支分化、Active Mask 与 Warp 级原语](https://hd9568.github.io/blog/cuda-warp-divergence-active-mask-shuffle/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**理解 Warp Divergence 的原因与代价，掌握 Active Mask 和 __shfl_sync 等 Warp 级原语的基本用法。

#### 一、核心结论

Warp 是 CUDA 性能优化中非常核心的概念。

- NVIDIA GPU 通常 32 个线程组成一个 Warp。

- Warp 内线程以 SIMT 方式执行同一条指令。

- 如果同一个 Warp 内线程走不同分支，就会发生 Warp Divergence。

- 分支分化会让不同路径串行执行，降低 Warp 执行效率。

- Active Mask 表示当前参与某个 Warp 级操作的线程集合。

- `__shfl_sync` 可以让 Warp 内线程直接交换寄存器值，常用于规约、广播和扫描。

#### 二、Warp 的执行模型

一个 Block 会被拆成多个 Warp。

```text
Block 128 threads
Warp 0: thread 0  - 31
Warp 1: thread 32 - 63
Warp 2: thread 64 - 95
Warp 3: thread 96 - 127
```

Warp Scheduler 按 Warp 发射指令。理想情况下，一个 Warp 内 32 个线程执行同一条指令，只是处理不同数据。

```cpp
int idx = blockIdx.x * blockDim.x + threadIdx.x;
y[idx] = x[idx] * 2.0f;
```

这类代码对每个线程路径一致，Warp 执行效率高。

#### 三、Warp Divergence 是什么

分支分化指同一个 Warp 内的线程走了不同分支。

```cpp
__global__ void branch_kernel(float* x, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        if (idx % 2 == 0) {
            x[idx] *= 2.0f;
        } else {
            x[idx] += 1.0f;
        }
    }
}
```

同一个 Warp 中，一半线程可能走 `if`，另一半走 `else`。硬件通常会：

1. 先执行 `if` 路径，屏蔽不走该路径的线程。

1. 再执行 `else` 路径，屏蔽另一批线程。

1. 最后汇合。

看起来是并行线程，实际分支路径会被串行化。

##### 边界检查也会产生分化吗

会，但通常只影响最后一个 Block 或最后一个 Warp。

```cpp
if (idx < n) {
    y[idx] = x[idx];
}
```

这种分化一般可以接受，因为它是越界保护必须付出的成本。

#### 四、如何减少分支分化

##### 1. 让同一 Warp 内线程尽量走同一路径

如果数据可以重排，把相同处理逻辑的数据放在相邻位置，能减少 Warp 内分化。

##### 2. 用算术表达式替代简单分支

```cpp
// 分支写法。
if (x > 0) {
    y = x;
} else {
    y = 0;
}
// 某些场景可改为无分支形式。
y = max(x, 0.0f);
```

是否更快要看编译器和硬件，不能机械替换。

##### 3. 避免线程粒度过细的复杂控制流

如果每个线程都执行完全不同的逻辑，GPU 的 SIMT 优势会下降。

##### 4. 把分支提升到 Warp 或 Block 粒度

如果一个条件对整个 Block 都相同，分支不会造成 Warp 内分化。

```cpp
if (blockIdx.x < boundary_block) {
    // 整个 block 走同一路径，分化少。
}
```

#### 五、Active Mask

Active Mask 是一个 32-bit 掩码，表示 Warp 中哪些 lane 参与当前操作。

```cpp
unsigned mask = __activemask();
```

每个 bit 对应一个 lane。如果某个线程处于活跃状态，对应 bit 为 1。

在使用 `__shfl_sync`、`__ballot_sync` 等同步 Warp 原语时，需要传入 mask：

```cpp
unsigned mask = __activemask();
```

这让硬件知道哪些线程必须参与该操作，避免分支场景下错误同步。

#### 六、Warp 级原语 __shfl_sync

`__shfl_sync` 可以让 Warp 内线程直接读取其他 lane 的寄存器值，不需要 shared memory。

```cpp
int value = threadIdx.x;
unsigned mask = 0xffffffff;
// 每个线程读取 lane 0 的 value。
int from_lane0 = __shfl_sync(mask, value, 0);
```

常见变体：

- `__shfl_sync(mask, val, srcLane)`：读取指定 lane。

- `__shfl_down_sync(mask, val, delta)`：读取当前 lane + delta 的值。

- `__shfl_up_sync(mask, val, delta)`：读取当前 lane - delta 的值。

- `__shfl_xor_sync(mask, val, laneMask)`：按 XOR 模式交换。

这些原语适合 Warp 内规约、广播、前缀和等操作。

#### 七、Warp 级规约示例

下面代码把一个 Warp 内的 `val` 求和，最终 lane 0 得到总和。

```cpp
__inline__ __device__ float warp_reduce_sum(float val) {
    // 0xffffffff 表示 32 个 lane 都参与。
    unsigned mask = 0xffffffff;
    // 每一步让当前线程读取后面 offset 个 lane 的值并累加。
    // offset: 16 -> 8 -> 4 -> 2 -> 1
    for (int offset = 16; offset > 0; offset /= 2) {
        val += __shfl_down_sync(mask, val, offset);
    }
    return val;
}
```

使用方式：

```cpp
__global__ void reduce_one_warp(const float* x, float* out) {
    int lane = threadIdx.x % 32;
    float val = x[threadIdx.x];
    float sum = warp_reduce_sum(val);
    // 只有 lane 0 写出结果，避免多个线程重复写。
    if (lane == 0) {
        out[blockIdx.x] = sum;
    }
}
```

相比 shared memory 规约，Warp shuffle 的优势是：

- 不需要 shared memory 存中间值。

- 不需要 `__syncthreads()`。

- 寄存器间交换更轻量。

#### 八、面试回答模板

如果问题是“什么是 Warp Divergence”，可以这样回答：

1. Warp 是 GPU 调度基本单位，NVIDIA 通常 32 个线程一个 Warp。

1. 同一个 Warp 内线程执行同一条指令，但处理不同数据。

1. 如果 Warp 内线程走不同分支，就会发生分支分化。

1. 硬件会分别执行不同路径，并屏蔽不在当前路径上的线程，导致执行效率下降。

1. 避免方式包括数据重排、减少线程级复杂分支、用无分支表达式、把分支提升到 Warp 或 Block 粒度。

如果问题是“__shfl_sync 有什么用”，可以回答：

1. 它用于 Warp 内线程之间交换寄存器值。

1. 常用于 Warp 级规约、广播和扫描。

1. 相比 shared memory，它减少了共享内存读写和同步开销。

1. 使用时需要传入 mask，表示参与操作的活跃 lane 集合。

#### 九、总结

Warp 机制决定了 CUDA 线程不是完全独立执行：

- Warp 内路径一致，执行效率高。

- Warp 内路径分化，分支会串行化。

- Active Mask 用于描述参与同步原语的线程集合。

- `__shfl_sync` 是高性能 Warp 内通信的重要工具。

## 3 内存、并发与执行优化

### CUDA Global Memory：合并访问、对齐与带宽优化

> 原文：[CUDA Global Memory：合并访问、对齐与带宽优化](https://hd9568.github.io/blog/cuda-global-memory-coalescing/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**理解 Global Memory 的高延迟特性，讲清 Coalesced Memory Access、对齐要求和常见访存优化写法。

#### 一、核心结论

Global Memory 容量大，但延迟高，是 CUDA 性能优化中最常见瓶颈。

- Global Memory 是 GPU 显存，所有线程都可以访问。

- 合并访问指一个 Warp 内线程访问连续、对齐的地址，硬件能用更少内存事务完成访问。

- 不连续、跨步很大、乱序访问会降低带宽利用率。

- 访问模式通常比单个线程里的指令数量更重要。

- 性能优化目标是让一个 Warp 的访存尽量连续、对齐、少重复。

#### 二、Global Memory 的特点

Global Memory 可以理解为 GPU 的主存。

```text
Thread -> L1/L2 Cache -> Global Memory
```

它的特点：

- 容量大。

- 延迟高。

- 带宽高但需要合适访问模式才能打满。

- 所有 Block 都能访问。

很多 kernel 是 Memory-bound：算术操作不多，主要时间花在读写显存。

#### 三、合并访问是什么

NVIDIA GPU 以 Warp 为单位组织访存。一个 Warp 里 32 个线程如果访问连续地址，硬件可以把这些访问合并成少量内存事务。

理想访问：

```text
lane 0 -> x[0]
lane 1 -> x[1]
lane 2 -> x[2]
...
lane31 -> x[31]
```

这种模式对 `float` 来说是 32 个连续 4 字节，共 128 字节，通常很适合硬件合并。

差访问：

```text
lane 0 -> x[0]
lane 1 -> x[1024]
lane 2 -> x[2048]
...
```

每个 lane 访问相隔很远，内存事务变多，带宽利用率下降。

#### 四、连续访问示例

```cpp
__global__ void copy_kernel(const float* __restrict__ input,
                            float* __restrict__ output,
                            int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // 相邻线程访问相邻元素，通常能形成合并访问。
        output[idx] = input[idx];
    }
}
```

如果 `blockDim.x` 是 32 的倍数，Warp 内线程的 `idx` 通常连续。这个访问模式对 Global Memory 友好。

#### 五、非合并访问问题

##### 跨步访问

```cpp
__global__ void strided_copy(const float* input, float* output, int n, int stride) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // 相邻线程访问相隔 stride 的元素。
        output[idx] = input[idx * stride];
    }
}
```

如果 `stride` 很大，一个 Warp 会访问很多分散地址，导致内存事务数量增加。

##### 矩阵按列访问

C/C++ 行主序矩阵中，连续的是同一行的相邻列。

```cpp
// 好：相邻线程访问同一行相邻列。
int row = blockIdx.y;
int col = blockIdx.x * blockDim.x + threadIdx.x;
y[row * width + col] = x[row * width + col];
```

如果让相邻线程访问不同行同一列，地址会相隔 `width`，容易不合并。

#### 六、对齐与向量化访问

对齐指访问地址最好落在硬件喜欢的边界上。例如 4 字节 `float` 按 4 字节对齐，`float4` 按 16 字节对齐。

向量化访问可以一次读写多个元素：

```cpp
__global__ void copy_float4(const float4* input, float4* output, int n4) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n4) {
        // 每个线程一次拷贝 4 个 float，要求地址满足 float4 对齐。
        output[idx] = input[idx];
    }
}
```

向量化可能减少指令数，但前提是：

- 数据地址对齐。

- 数据长度能按向量宽度处理。

- 尾部元素需要单独处理。

#### 七、优化 checklist

- 相邻线程是否访问相邻地址。

- Block 线程数是否是 32 的倍数。

- 是否存在跨步访问或转置式访问。

- 是否能调整数据布局，让读写更连续。

- 是否存在重复读取，可否用 Shared Memory 复用。

- 是否能使用 `__restrict__` 帮助编译器优化别名分析。

- 是否能用 `float2/float4` 做安全向量化访问。

#### 八、面试回答模板

如果问题是“什么是合并访问”，可以这样回答：

1. 合并访问是指一个 Warp 内线程访问连续且对齐的 Global Memory 地址。

1. 硬件可以把多个线程的访问合并成更少内存事务，提高带宽利用率。

1. 相邻线程访问相邻元素通常是合并访问，跨步或随机访问会破坏合并。

1. CUDA 访存优化的核心是让 Warp 级访问连续、对齐、少重复。

1. 对矩阵类 kernel，要特别注意行主序/列主序和线程映射方式。

#### 九、总结

Global Memory 优化的第一原则是看 Warp 的访存模式，而不是只看单个线程代码。相邻线程访问连续数据，通常比每个线程写很复杂的局部优化更重要。

### CUDA Shared Memory：Bank Conflict、Padding 与 Swizzle

> 原文：[CUDA Shared Memory：Bank Conflict、Padding 与 Swizzle](https://hd9568.github.io/blog/cuda-shared-memory-bank-conflict/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**理解 Shared Memory 的 Bank 组织方式，讲清 Bank Conflict 的产生原因，以及 Padding 和 Swizzle 的解决思路。

#### 一、核心结论

Shared Memory 是 Block 内线程共享的高速片上内存，但访问方式不合理会产生 Bank Conflict。

- Shared Memory 延迟远低于 Global Memory。

- 它常用于缓存数据、复用数据、做 Block 内协作。

- Shared Memory 被划分成多个 Bank，线程访问不同 Bank 可以并行。

- 同一个 Warp 中多个线程访问同一个 Bank 的不同地址，会产生 Bank Conflict。

- Padding 通过改变行跨度，打散 Bank 映射。

- Swizzle 通过重排索引，让访问更均匀落到不同 Bank。

#### 二、Shared Memory 是什么

Shared Memory 属于一个 Block，Block 内所有线程可见。

```cpp
__global__ void kernel(float* out) {
    __shared__ float tile[256];
    int tid = threadIdx.x;
    tile[tid] = static_cast<float>(tid);
    // 确保所有线程都完成写入，再读取其他线程写的数据。
    __syncthreads();
    out[blockIdx.x * blockDim.x + tid] = tile[255 - tid];
}
```

它常用于：

- GEMM Tiling。

- Reduction 中间结果。

- Stencil 邻域缓存。

- 避免重复 Global Memory 访问。

#### 三、Bank 是什么

Shared Memory 内部被划分为多个 Bank。常见理解是 32 个 Bank，对应一个 Warp 的 32 个 lane。

对 4 字节数据，一个简化映射是：

```text
bank_id = address_index % 32
```

如果 Warp 内 32 个线程分别访问：

```text
tile[0], tile[1], tile[2], ..., tile[31]
```

它们通常落到 32 个不同 Bank，可以并行访问。

#### 四、Bank Conflict 如何产生

如果多个线程访问同一个 Bank 的不同地址，就会产生冲突，访问被拆成多次。

典型场景：按列访问二维 shared memory。

```cpp
__shared__ float tile[32][32];
int lane = threadIdx.x;
float x = tile[lane][0];
```

行主序下，`tile[lane][0]` 的地址间隔是 32 个 float。简化映射：

```text
bank = (lane * 32 + 0) % 32 = 0
```

所有 lane 都访问 Bank 0 的不同地址，发生 32 路 Bank Conflict。

##### 广播例外

如果多个线程访问同一个地址，硬件通常可以广播，不一定造成冲突。

```cpp
float x = tile[0][0];  // 所有线程读同一个地址，通常可广播。
```

#### 五、Padding 解决冲突

Padding 是在每行末尾多加一列，改变行跨度。

```cpp
__shared__ float tile[32][33];
```

再按列访问：

```cpp
int lane = threadIdx.x;
float x = tile[lane][0];
```

简化映射：

```text
bank = (lane * 33 + 0) % 32 = lane % 32
```

32 个 lane 落到不同 Bank，冲突消失。

#### 六、Swizzle 的思路

Swizzle 是通过改变索引映射，让物理存储位置和逻辑位置不同，从而减少冲突。

简化例子：

```cpp
int swizzled_col = col ^ (row & 31);
tile[row][swizzled_col] = value;
```

读取时使用相同规则还原对应位置。Swizzle 常用于更复杂的矩阵乘和 Tensor Core 数据布局中。

它的本质不是多存一列，而是用重排让不同线程访问分散到不同 Bank。

#### 七、矩阵转置示例

朴素转置会有非合并写或非合并读。优化思路是先把 tile 读入 Shared Memory，再转置写出。

```cpp
#define TILE 32
__global__ void transpose(const float* input, float* output, int width, int height) {
    // 多加 1 列，避免按列读取 shared memory 时发生 bank conflict。
    __shared__ float tile[TILE][TILE + 1];
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    if (x < width && y < height) {
        // Global Memory 连续读。
        tile[threadIdx.y][threadIdx.x] = input[y * width + x];
    }
    __syncthreads();
    int out_x = blockIdx.y * TILE + threadIdx.x;
    int out_y = blockIdx.x * TILE + threadIdx.y;
    if (out_x < height && out_y < width) {
        // 从 shared memory 转置读取，再连续写到 Global Memory。
        output[out_y * height + out_x] = tile[threadIdx.x][threadIdx.y];
    }
}
```

关键点：

- `tile[TILE][TILE + 1]` 通过 padding 避免列访问冲突。

- `__syncthreads()` 保证 tile 完整写入后再读取。

- 读写 Global Memory 时尽量保持连续访问。

#### 八、面试回答模板

如果问题是“什么是 Bank Conflict”，可以这样回答：

1. Shared Memory 被划分成多个 Bank，Warp 内线程访问不同 Bank 可以并行。

1. 如果同一个 Warp 内多个线程访问同一 Bank 的不同地址，就会发生 Bank Conflict。

1. 冲突会让访问串行化，降低 shared memory 带宽。

1. 常见解决方法是 Padding，把二维数组列数从 32 改成 33，打散 Bank 映射。

1. 更复杂场景可以用 Swizzle 重排索引，减少冲突。

#### 九、总结

Shared Memory 优化不是“用了就快”。它需要同时满足：

- 减少 Global Memory 重复访问。

- Block 内同步正确。

- 访问模式避免 Bank Conflict。

- Padding 和 Swizzle 是解决冲突的常见工具。

### CUDA Registers：寄存器使用、Spilling 与 Local Memory

> 原文：[CUDA Registers：寄存器使用、Spilling 与 Local Memory](https://hd9568.github.io/blog/cuda-registers-spilling-local-memory/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**理解寄存器为什么影响 Occupancy，讲清 Register Spilling 如何把变量溢出到 Local Memory 并拖慢 kernel。

#### 一、核心结论

寄存器是线程私有、速度最快的存储资源，但数量有限。

- 每个线程都有自己的寄存器。

- 单线程使用寄存器越多，一个 SM 能同时驻留的线程或 Warp 可能越少。

- 寄存器不够时，编译器会把部分变量溢出到 Local Memory。

- Local Memory 名字里有 local，但物理上通常在 Global Memory 中，延迟很高。

- 优化目标不是盲目减少寄存器，而是在寄存器使用、Occupancy 和指令数之间平衡。

#### 二、寄存器是什么

寄存器用于保存线程的局部变量和中间结果。

```cpp
__global__ void saxpy(const float* x, float* y, float a, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // idx、vx、vy 等通常会放在寄存器中。
        float vx = x[idx];
        float vy = y[idx];
        y[idx] = a * vx + vy;
    }
}
```

寄存器访问很快，不需要显式声明，也不需要 `__syncthreads()`。

#### 三、寄存器与 Occupancy

一个 SM 的寄存器总数有限。假设：

```text
SM total registers = fixed budget
registers per thread = R
threads resident on SM <= total_registers / R
```

如果每个线程用很多寄存器，SM 上能同时驻留的 Warp 数会下降，Occupancy 下降。

但 Occupancy 不是越高越好：

- Occupancy 太低，可能无法隐藏访存延迟。

- 为了提高 Occupancy 强行减少寄存器，可能增加指令或 spilling，反而变慢。

#### 四、Register Spilling 是什么

如果编译器发现寄存器不够，会把某些变量放到 Local Memory。

例如大量局部数组或变量可能导致 spilling：

```cpp
__global__ void heavy_kernel(float* out, const float* in) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // 局部数组如果索引不能在编译期确定，可能无法完全放进寄存器。
    float tmp[64];
    #pragma unroll
    for (int i = 0; i < 64; ++i) {
        tmp[i] = in[idx + i];
    }
    float sum = 0.0f;
    for (int i = 0; i < 64; ++i) {
        sum += tmp[i];
    }
    out[idx] = sum;
}
```

如果 `tmp` 溢出到 Local Memory，每次访问都可能变成高延迟内存访问。

#### 五、Local Memory 的误区

Local Memory 听起来像“本地高速内存”，但在 CUDA 中它通常表示线程私有的内存空间，物理上常位于 Global Memory。

常见导致 Local Memory 的情况：

- 寄存器溢出。

- 过大的线程局部数组。

- 不能静态解析索引的局部数组。

- 大结构体局部变量。

因此，看到 Local Memory 访问要警惕，它可能意味着性能问题。

#### 六、如何观察和优化

##### 1. 查看编译信息

```bash
nvcc -Xptxas -v kernel.cu -o kernel
```

可能看到类似信息：

```text
ptxas info : Used 64 registers, 0 bytes spill stores, 0 bytes spill loads
```

重点看：

- Used registers。

- spill stores。

- spill loads。

##### 2. 用 Nsight Compute

关注指标：

- Registers Per Thread。

- Achieved Occupancy。

- Local Memory Load/Store。

- Stall 原因。

##### 3. 减少不必要局部变量生命周期

```cpp
// 不要让很多临时变量同时活着。
float a = ...;
float b = ...;
float c = a + b;
```

可以把计算拆成更短生命周期，帮助编译器回收寄存器。

##### 4. 控制展开程度

循环完全展开可能增加寄存器压力。`#pragma unroll` 不是一定越多越好。

```cpp
#pragma unroll 4
for (int i = 0; i < 32; ++i) {
    sum += x[i];
}
```

##### 5. 使用 launch_bounds 或 maxrregcount 要谨慎

```cpp
__launch_bounds__(256, 2)
__global__ void kernel(...) {
}
```

或者：

```bash
nvcc --maxrregcount=64 kernel.cu
```

这些方式可以限制寄存器使用，但如果限制过严，可能导致更多 spilling。

#### 七、面试回答模板

如果问题是“寄存器溢出为什么慢”，可以这样回答：

1. 寄存器是线程私有的高速存储，但每个 SM 的寄存器总量有限。

1. 如果单线程使用寄存器太多，会降低 SM 上可驻留的 Warp 数，影响 Occupancy。

1. 如果寄存器不够，编译器会把部分变量 spill 到 Local Memory。

1. Local Memory 虽然名字叫 local，但通常在 Global Memory 中，访问延迟高。

1. 优化时要结合 ptxas、Nsight Compute，看寄存器数、spill loads/stores 和实际性能。

#### 八、总结

寄存器优化的关键是平衡：

- 寄存器多：减少重复访存和指令，但可能降低 Occupancy。

- 寄存器少：Occupancy 可能提高，但可能增加 spilling。

- 最终应通过 profiling 决定，而不是只追求某个单一指标。

### CUDA Constant Memory 与 Texture Memory：只读数据的访存选择

> 原文：[CUDA Constant Memory 与 Texture Memory：只读数据的访存选择](https://hd9568.github.io/blog/cuda-constant-texture-memory/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**理解 Constant Memory 和 Texture Memory 的适用场景，掌握只读广播数据、空间局部性数据的访存优化思路。

#### 一、核心结论

Constant Memory 和 Texture Memory 都适合只读数据，但优化目标不同。

- Constant Memory 适合所有线程读取相同地址的广播场景。

- Texture Memory 适合具有空间局部性的只读访问，例如图像、二维数据、插值采样。

- 如果 Warp 内线程读取不同 Constant Memory 地址，访问可能串行化。

- 现代 GPU 还有只读数据缓存路径，普通 `const __restrict__` 指针也可能表现很好。

- 选择特殊内存前，应先确认访问模式。

#### 二、为什么需要特殊只读内存

很多 kernel 会访问只读参数：

- 卷积权重。

- 标量配置。

- 查表数据。

- 图像或二维纹理。

- 小型常量数组。

如果访问模式稳定，可以让硬件使用更合适的缓存路径，减少 Global Memory 压力。

#### 三、Constant Memory

Constant Memory 用 `__constant__` 声明。

```cpp
// 设备端常量数组，大小通常有限，适合小型只读参数。
__constant__ float kWeights[256];
```

Host 端拷贝数据：

```cpp
float h_weights[256];
// 初始化 h_weights ...
cudaMemcpyToSymbol(kWeights, h_weights, sizeof(h_weights));
```

Kernel 使用：

```cpp
__global__ void apply_weight(const float* x, float* y, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // 如果 Warp 内线程读取同一个 kWeights 下标，constant cache 可以广播。
        y[idx] = x[idx] * kWeights[0];
    }
}
```

##### Constant Memory 适合广播

好模式：

```cpp
float scale = kWeights[0];  // Warp 内所有线程读同一地址。
```

差模式：

```cpp
float w = kWeights[threadIdx.x];  // Warp 内线程读不同地址，可能串行化。
```

因此，Constant Memory 最适合“很多线程读同一个小常量”。

#### 四、Texture Memory

Texture Memory 更偏向图像和二维/三维空间局部性访问。它通过 texture cache 优化只读数据访问，支持一些硬件采样能力。

现代 CUDA 中常使用 texture object。

伪代码结构：

```cpp
// Host 端创建 cudaTextureObject_t，把 CUDA array 或线性内存绑定给 texture。
// Kernel 中通过 tex2D<float>(tex, x, y) 读取。
```

Kernel 侧示意：

```cpp
__global__ void sample_kernel(cudaTextureObject_t tex, float* out, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x < width && y < height) {
        // tex2D 适合二维空间局部性访问。
        float value = tex2D<float>(tex, x + 0.5f, y + 0.5f);
        out[y * width + x] = value;
    }
}
```

Texture Memory 的典型优势：

- 对二维局部性访问友好。

- 支持地址模式和过滤模式。

- 适合图像处理、采样、查表类场景。

#### 五、二者如何选择

| 场景 | 更适合 |
| --- | --- |
| 所有线程读同一个小参数 | Constant Memory |
| Warp 内线程读不同常量地址 | 不一定适合 Constant Memory |
| 二维图像采样 | Texture Memory |
| 普通连续数组只读 | Global Memory + 只读缓存路径可能足够 |
| 大型模型权重连续读 | 通常关注 coalescing、L2、Tensor Core 数据流 |

对于 AI 算子，权重通常很大，Constant Memory 容量不够。它更适合小表、小配置、广播标量。

#### 六、常见误区

##### 1. Constant Memory 不是所有只读访问都快

如果 Warp 内每个线程读不同地址，它可能被串行化，反而不快。

##### 2. Texture Memory 不是必须用于所有二维数据

如果二维数据访问本身已经合并、连续，普通 Global Memory 加缓存也可能很好。

##### 3. 特殊内存不能替代数据布局优化

访存优化第一步仍然是看访问模式：连续、对齐、复用、少重复。

#### 七、面试回答模板

如果问题是“Constant Memory 和 Texture Memory 适用场景”，可以这样回答：

1. Constant Memory 适合小型只读数据，尤其是 Warp 内线程读取同一地址的广播场景。

1. 如果 Warp 内线程读取不同 constant 地址，访问可能串行化。

1. Texture Memory 适合有空间局部性的只读访问，例如图像、二维采样和查表。

1. 现代 GPU 普通只读 Global Memory 也有缓存路径，是否使用特殊内存要看访问模式和 profiling。

1. AI 算子中大权重通常不放 Constant Memory，小参数和配置更适合。

#### 八、总结

Constant Memory 和 Texture Memory 的本质是给特定只读访问模式提供更合适的缓存路径。优化前先判断访问模式：广播读选 Constant，二维局部性读考虑 Texture，连续大数组优先保证合并访问和数据复用。

### CUDA Streams：异步执行、Event 同步与计算传输重叠

> 原文：[CUDA Streams：异步执行、Event 同步与计算传输重叠](https://hd9568.github.io/blog/cuda-streams-events-overlap/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**理解 CUDA Stream 的异步执行模型，掌握 Event 同步、cudaMemcpyAsync 和 Ping-Pong Buffer 实现计算与数据传输重叠。

#### 一、核心结论

CUDA Stream 用来组织 GPU 上的异步任务队列。

- 同一个 Stream 内任务按提交顺序执行。

- 不同 Stream 中的任务在资源允许时可以并发。

- Kernel launch 默认是异步的，Host 不会等待 kernel 完成。

- `cudaEvent_t` 可以记录某个 Stream 的进度，并让其他 Stream 等待。

- 使用 pinned memory 和 `cudaMemcpyAsync` 才更容易实现 H2D/D2H 与 kernel 的重叠。

- Ping-Pong Buffer 用两套缓冲交替传输和计算，隐藏数据传输延迟。

#### 二、Stream 是什么

Stream 是 GPU 任务队列。

```text
Stream 0: H2D copy -> Kernel -> D2H copy
Stream 1: H2D copy -> Kernel -> D2H copy
```

同一个 Stream 内保持顺序；不同 Stream 之间没有默认顺序依赖。

创建 Stream：

```cpp
cudaStream_t stream;
cudaStreamCreate(&stream);
kernel<<<grid, block, 0, stream>>>(args);
cudaStreamDestroy(stream);
```

Kernel 启动语法中第 4 个参数就是 Stream。

#### 三、默认流与异步执行

Kernel launch 对 Host 异步：

```cpp
my_kernel<<<grid, block>>>(x);
// Host 线程会继续往下执行，不会自动等 kernel 完成。
```

如果要等待 GPU 完成：

```cpp
cudaDeviceSynchronize();
```

更细粒度等待某个 Stream：

```cpp
cudaStreamSynchronize(stream);
```

使用异步拷贝：

```cpp
cudaMemcpyAsync(d_x, h_x, bytes, cudaMemcpyHostToDevice, stream);
```

注意：Host 内存最好是 pinned memory，否则异步拷贝和重叠效果可能受限。

```cpp
float* h_x = nullptr;
cudaHostAlloc(&h_x, bytes, cudaHostAllocDefault);  // pinned host memory
```

#### 四、Event 同步

Event 可以记录一个 Stream 的执行位置。

```cpp
cudaEvent_t event;
cudaEventCreate(&event);
// 在 stream_a 中记录事件。
cudaEventRecord(event, stream_a);
// stream_b 等待 event 完成后再继续。
cudaStreamWaitEvent(stream_b, event, 0);
cudaEventDestroy(event);
```

典型用途：

- 测量 GPU 时间。

- 建立不同 Stream 之间的依赖。

- 避免全局 `cudaDeviceSynchronize()` 带来的过度同步。

测量时间：

```cpp
cudaEvent_t start, stop;
cudaEventCreate(&start);
cudaEventCreate(&stop);
cudaEventRecord(start, stream);
kernel<<<grid, block, 0, stream>>>(args);
cudaEventRecord(stop, stream);
cudaEventSynchronize(stop);
float ms = 0.0f;
cudaEventElapsedTime(&ms, start, stop);
```

#### 五、计算与数据传输重叠

理想流水线：

```text
Time --->
Stream 0: H2D chunk0 -> Kernel chunk0 -> D2H chunk0
Stream 1:             H2D chunk1 -> Kernel chunk1 -> D2H chunk1
```

当 GPU 支持 copy engine 和 compute engine 并发时，传输和计算可以重叠。

基本条件：

- 使用不同 Stream。

- 使用 `cudaMemcpyAsync`。

- Host 内存是 pinned memory。

- Kernel 计算时间和拷贝时间都有足够规模。

- 硬件支持并发 copy/compute。

#### 六、Ping-Pong Buffer

Ping-Pong Buffer 使用两套 device buffer 交替处理数据块。

```cpp
constexpr int kStreams = 2;
cudaStream_t streams[kStreams];
float* d_in[kStreams];
float* d_out[kStreams];
for (int i = 0; i < kStreams; ++i) {
    cudaStreamCreate(&streams[i]);
    cudaMalloc(&d_in[i], chunk_bytes);
    cudaMalloc(&d_out[i], chunk_bytes);
}
for (int chunk = 0; chunk < num_chunks; ++chunk) {
    int s = chunk % kStreams;
    // 第 s 套 buffer 处理当前 chunk。
    cudaMemcpyAsync(d_in[s], h_in + chunk * chunk_elems,
                    chunk_bytes, cudaMemcpyHostToDevice, streams[s]);
    // 同一个 stream 内，kernel 会等当前 H2D 完成。
    kernel<<<grid, block, 0, streams[s]>>>(d_in[s], d_out[s], chunk_elems);
    cudaMemcpyAsync(h_out + chunk * chunk_elems, d_out[s],
                    chunk_bytes, cudaMemcpyDeviceToHost, streams[s]);
}
for (int i = 0; i < kStreams; ++i) {
    cudaStreamSynchronize(streams[i]);
}
```

注释里的关键点：

- 同一 Stream 内顺序保证，不需要在 H2D 和 Kernel 之间手动同步。

- 不同 Stream 之间可重叠执行。

- 两套 buffer 避免下一块数据覆盖上一块还没用完的 buffer。

#### 七、常见坑

##### 1. 用 pageable memory 期待重叠

普通 `malloc/new` 的 Host 内存不一定能真正异步传输。使用 pinned memory 更稳。

##### 2. 过度 cudaDeviceSynchronize

```cpp
cudaMemcpyAsync(..., stream);
cudaDeviceSynchronize();  // 过度同步，破坏流水线
kernel<<<..., stream>>>(...);
```

应尽量用 Stream 或 Event 做局部同步。

##### 3. Chunk 太小

Chunk 太小会导致 launch overhead 和拷贝开销占比过高，重叠效果不明显。

##### 4. 多 Stream 不一定更快

如果 kernel 已经占满全部 SM，或者拷贝引擎已满，多 Stream 可能没有收益。

#### 八、面试回答模板

如果问题是“CUDA Stream 如何实现重叠”，可以这样回答：

1. Stream 是 GPU 异步任务队列，同一 Stream 顺序执行，不同 Stream 可并发。

1. Kernel launch 和 `cudaMemcpyAsync` 都可以异步提交。

1. 要实现 H2D/D2H 与 kernel 重叠，通常需要不同 Stream、pinned memory、异步拷贝和硬件支持。

1. Event 用于记录 Stream 进度，也可以让一个 Stream 等待另一个 Stream 的事件。

1. Ping-Pong Buffer 用两套缓冲交替传输和计算，减少数据传输对总时间的影响。

#### 九、总结

Stream 优化的目标是把串行流程改成流水线：

```text
传输 chunk i+1 的同时，计算 chunk i
```

它适合数据分块明显、传输成本可观、计算和拷贝可并发的场景。实际收益必须用 Nsight Systems 看时间线验证。

### CUDA 循环展开：#pragma unroll 的原理与取舍

> 原文：[CUDA 循环展开：#pragma unroll 的原理与取舍](https://hd9568.github.io/blog/cuda-loop-unrolling-pragma-unroll/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**理解循环展开如何减少分支和索引开销，分析 #pragma unroll 在 CUDA kernel 中对指令数、寄存器和性能的影响。

#### 一、核心结论

循环展开是用更多指令体积换更少循环控制开销和更高指令级并行。

- `#pragma unroll` 提示编译器展开循环。

- 完全展开能减少循环分支、计数器更新和索引计算。

- 展开可能增加寄存器压力和指令缓存压力。

- 对固定小循环，展开常有收益。

- 对大循环或寄存器紧张 kernel，过度展开可能变慢。

- 是否展开应结合 ptxas 信息和 Nsight 指标验证。

#### 二、循环展开是什么

普通循环：

```cpp
float sum = 0.0f;
for (int i = 0; i < 4; ++i) {
    sum += x[i];
}
```

展开后相当于：

```cpp
float sum = 0.0f;
sum += x[0];
sum += x[1];
sum += x[2];
sum += x[3];
```

省掉了循环判断和计数器更新，也让编译器更容易调度指令。

#### 三、#pragma unroll 的用法

完全展开：

```cpp
#pragma unroll
for (int i = 0; i < 8; ++i) {
    sum += x[i];
}
```

指定展开因子：

```cpp
#pragma unroll 4
for (int i = 0; i < 32; ++i) {
    sum += x[i];
}
```

禁止展开：

```cpp
#pragma unroll 1
for (int i = 0; i < n; ++i) {
    sum += x[i];
}
```

编译器可能会根据循环边界是否已知、代码大小、优化等级决定是否采纳。

#### 四、为什么可能变快

##### 1. 减少循环控制开销

展开后少了比较、跳转、计数器更新。

##### 2. 增加指令级并行

多个独立操作暴露给编译器后，可以更好调度。

```cpp
#pragma unroll
for (int i = 0; i < 4; ++i) {
    acc0 += a[i];
    acc1 += b[i];
}
```

##### 3. 固定索引更容易优化

编译器看到 `x[0]`、`x[1]` 这种固定索引，可能生成更高效指令。

#### 五、为什么也可能变慢

##### 1. 寄存器压力增加

展开后更多变量同时活跃，可能增加寄存器使用，降低 Occupancy，甚至导致 spilling。

##### 2. 代码体积变大

指令太多可能影响指令缓存和调度。

##### 3. 访存瓶颈未解决

如果 kernel 是明显 Memory-bound，展开计算循环不一定提升整体性能。

##### 4. 编译器本来就会自动展开

手写 `#pragma unroll` 不一定比编译器自动优化更好。

#### 六、Reduction 中的展开示例

Block 内规约时，最后一个 Warp 可以不再用 `__syncthreads()`，配合展开减少循环开销。

```cpp
__inline__ __device__ float warp_reduce_sum(float v) {
    unsigned mask = 0xffffffff;
    // 固定 5 步规约，完全展开后没有循环控制开销。
    v += __shfl_down_sync(mask, v, 16);
    v += __shfl_down_sync(mask, v, 8);
    v += __shfl_down_sync(mask, v, 4);
    v += __shfl_down_sync(mask, v, 2);
    v += __shfl_down_sync(mask, v, 1);
    return v;
}
```

这里不一定需要 `#pragma unroll`，因为手动写成了固定步骤。很多高性能规约都会显式展开最后几个阶段。

#### 七、实践建议

- 固定小循环可以尝试完全展开。

- 大循环先尝试 `#pragma unroll 4` 或 `8`，不要默认完全展开。

- 关注 `Used registers` 和 `spill loads/stores`。

- 对 Memory-bound kernel，优先优化访存，再考虑展开。

- 对 Compute-bound kernel，展开可能更有价值。

- 每次修改都用 profiling 对比，不凭感觉判断。

#### 八、面试回答模板

如果问题是“#pragma unroll 有什么作用”，可以这样回答：

1. 它提示编译器展开循环，把循环体复制多份。

1. 好处是减少循环控制开销，暴露更多指令级并行，固定索引更容易优化。

1. 坏处是代码体积和寄存器压力增加，可能降低 Occupancy 或造成 spilling。

1. 小固定循环通常适合展开，大循环要谨慎。

1. 最终要结合 ptxas 和 Nsight 指标判断是否真的变快。

#### 九、总结

循环展开不是万能优化。它适合用在固定次数、小循环、计算密集或规约尾部。判断标准不是“展开越多越好”，而是看总时间、寄存器、Occupancy 和访存瓶颈是否改善。

### CUDA Kernel Fusion：为什么融合能减少显存带宽瓶颈

> 原文：[CUDA Kernel Fusion：为什么融合能减少显存带宽瓶颈](https://hd9568.github.io/blog/cuda-kernel-fusion-memory-bound/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**从 Memory-bound 和 Compute-bound 角度理解 Kernel Fusion，讲清融合如何减少中间结果读写以及可能带来的副作用。

#### 一、核心结论

Kernel Fusion 是把多个相邻算子合并成一个 kernel。

- 融合能减少中间结果写回 Global Memory 和再次读出。

- 融合能减少 kernel launch overhead。

- 对 Memory-bound 算子通常收益明显。

- 对 Compute-bound 算子，融合收益不一定大。

- 过度融合可能增加寄存器压力、降低 Occupancy、破坏复用或增加代码复杂度。

#### 二、Memory-bound 与 Compute-bound

Memory-bound：瓶颈在显存带宽。

```text
读写数据很多，计算很少
例如：y = x + bias、ReLU、简单逐元素操作
```

Compute-bound：瓶颈在计算吞吐。

```text
计算量很大，数据复用高
例如：大矩阵 GEMM
```

Kernel Fusion 主要优化 Memory-bound 场景，因为它减少显存读写。

#### 三、为什么多个 kernel 会慢

假设有两个操作：

```cpp
tmp = x + bias;
y = relu(tmp);
```

如果拆成两个 kernel：

```text
Kernel 1: read x, read bias, write tmp
Kernel 2: read tmp, write y
```

`tmp` 是中间结果，被写入 Global Memory 后又读回来。这个过程消耗显存带宽。

#### 四、Kernel Fusion 示例

拆分写法：

```cpp
__global__ void add_bias(const float* x, const float* bias, float* tmp, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        tmp[idx] = x[idx] + bias[idx];
    }
}
__global__ void relu(const float* tmp, float* y, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        y[idx] = tmp[idx] > 0.0f ? tmp[idx] : 0.0f;
    }
}
```

融合写法：

```cpp
__global__ void add_bias_relu(const float* x, const float* bias, float* y, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // 中间结果 v 保存在寄存器里，不写回 Global Memory。
        float v = x[idx] + bias[idx];
        y[idx] = v > 0.0f ? v : 0.0f;
    }
}
```

融合后访存从：

```text
read x + read bias + write tmp + read tmp + write y
```

变成：

```text
read x + read bias + write y
```

省掉了 `tmp` 的一次写和一次读。

#### 五、融合为什么可能加速

##### 1. 减少显存流量

对逐元素算子，显存读写往往比计算更贵。减少中间结果读写直接降低时间。

##### 2. 减少 kernel launch overhead

多个小 kernel 合成一个 kernel，可以减少 Host 提交开销。

##### 3. 中间值留在寄存器

融合后中间结果可以保存在寄存器里，避免 Global Memory 往返。

##### 4. 增加算术强度

算术强度是计算量和内存访问量的比值。

```text
Arithmetic Intensity = FLOPs / Bytes
```

融合减少 Bytes，如果 FLOPs 不变，算术强度提高，可能让 Memory-bound 更接近 Compute-bound。

#### 六、融合的副作用

##### 1. 寄存器压力增加

融合后一个线程同时保存更多中间变量，可能增加寄存器数量。

##### 2. Occupancy 降低

寄存器或 shared memory 使用增加，可能让 SM 上驻留的 Warp 变少。

##### 3. 代码复杂度增加

融合太多算子后，kernel 更难维护，也更难适配不同形状。

##### 4. 破坏库算子优化

有些单独算子已经由 cuBLAS/cuDNN 高度优化。手动融合可能反而不如库实现。

#### 七、常见融合场景

- Bias + Activation。

- Add + LayerNorm 前后处理。

- Scale + Mask + Softmax 某些阶段。

- Elementwise 链式操作。

- Transformer 中残差、bias、activation、dropout 的组合。

不太适合盲目融合的场景：

- 大 GEMM 主体。

- 计算密集且 Tensor Core 利用率已高的 kernel。

- 融合后寄存器压力明显爆炸的复杂逻辑。

#### 八、面试回答模板

如果问题是“为什么 Kernel Fusion 可以加速”，可以这样回答：

1. 很多逐元素算子是 Memory-bound，时间主要花在 Global Memory 读写。

1. 多个 kernel 拆开执行时，中间结果需要写回显存，再由下一个 kernel 读出。

1. Fusion 把多个操作合到一个 kernel，中间结果可以留在寄存器中，减少显存流量。

1. 它还能减少 kernel launch overhead，并提高算术强度。

1. 但融合不是越多越好，可能增加寄存器压力、降低 Occupancy、增加代码复杂度。

#### 九、总结

Kernel Fusion 的本质是减少不必要的内存往返。对 Memory-bound 的 elementwise 链路很有效；对 Compute-bound 的大算子，收益要具体分析。好的融合策略应该用 Nsight 指标验证显存流量和实际耗时是否下降。

## 4 Profiling 与性能诊断

### CUDA Profiling：Nsight Compute 与 Nsight Systems 常用指标

> 原文：[CUDA Profiling：Nsight Compute 与 Nsight Systems 常用指标](https://hd9568.github.io/blog/cuda-profiling-ncu-nsys/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**区分 NCU 和 NSYS 的定位，理解 Occupancy、Compute Workload、Memory Bandwidth 等常用指标如何指导 CUDA 优化。

#### 一、核心结论

Profiling 的目标不是收集指标，而是定位瓶颈。

- Nsight Systems（NSYS）看系统级时间线：CPU、GPU、Memcpy、Kernel、Stream 是否重叠。

- Nsight Compute（NCU）看单个 kernel 的细节：访存、计算、Occupancy、Warp stall 等。

- 先用 NSYS 找慢在哪里，再用 NCU 深挖具体 kernel。

- Occupancy 不等于性能，只表示 SM 上活跃 Warp 的比例。

- Memory Bandwidth 高且算力低，通常说明 Memory-bound。

- Compute Workload 高且访存不是瓶颈，可能是 Compute-bound。

#### 二、NSYS 和 NCU 的区别

| 工具 | 主要用途 | 关注对象 |
| --- | --- | --- |
| Nsight Systems | 系统级时间线 | 程序整体、CPU/GPU 并发、Memcpy、Stream |
| Nsight Compute | Kernel 级分析 | 单个 kernel 的访存、计算、occupancy、stall |

常见命令：

```bash
nsys profile -o report ./app
```

```bash
ncu --set full ./app
```

实际工程中，通常先 `nsys`，再对热点 kernel 用 `ncu`。

#### 三、先看时间线

NSYS 中重点看：

- GPU 是否有长时间空洞。

- Memcpy 是否和 Kernel 重叠。

- Kernel 是否很多且很短。

- CPU 是否频繁同步 GPU。

- 多 Stream 是否真的并发。

例如，如果时间线是：

```text
H2D -> Kernel -> D2H -> H2D -> Kernel -> D2H
```

说明传输和计算没有重叠。可以考虑 Streams 和 Ping-Pong Buffer。

如果时间线里很多小 kernel：

```text
kernel1 kernel2 kernel3 kernel4 ...
```

可能需要考虑 Kernel Fusion 或减少 launch overhead。

#### 四、Occupancy

Occupancy 表示一个 SM 上实际活跃 Warp 数与理论最大活跃 Warp 数的比例。

影响因素：

- 每个线程使用的寄存器数。

- 每个 Block 使用的 shared memory。

- Block 大小。

- 硬件最大线程/Block/Warp 限制。

Occupancy 低可能导致无法隐藏访存延迟，但 Occupancy 高不代表一定快。

例子：

```text
Kernel A: Occupancy 30%, 每个线程做大量计算，数据复用好，可能很快。
Kernel B: Occupancy 90%, 但访存完全随机，仍然可能很慢。
```

因此 Occupancy 只是线索，不是目标本身。

#### 五、Compute Workload

Compute Workload 相关指标用来判断计算单元是否繁忙。

关注：

- SM 是否忙。

- FP32/FP16/Tensor Core 利用率。

- 指令吞吐是否接近硬件上限。

- Warp stall 是否来自执行依赖或调度不足。

如果 Tensor Core 利用率低，但理论上应该用 Tensor Core，可能需要检查：

- 数据类型是否是 FP16/BF16/TF32/INT8 等合适类型。

- 矩阵维度是否满足库或硬件要求。

- 是否调用了正确的 cuBLAS/cuDNN/CUTLASS 路径。

#### 六、Memory Bandwidth

Memory Bandwidth 指标用于判断显存带宽利用情况。

关注：

- Global Load/Store Throughput。

- L2 Hit Rate。

- DRAM Throughput。

- Memory Workload Analysis。

- Load/Store 是否合并。

如果 DRAM 带宽接近上限，而计算单元利用率不高，通常是 Memory-bound。

优化方向：

- 合并访问。

- 减少重复读写。

- 用 Shared Memory 复用数据。

- Kernel Fusion 减少中间结果写回。

- 使用更紧凑数据类型。

#### 七、常见瓶颈判断

##### 1. GPU 时间线有空洞

可能原因：

- CPU 端准备数据慢。

- 频繁同步。

- Kernel launch 间隔大。

优化：减少同步、批量提交、使用 CUDA Graph、改进数据加载。

##### 2. Memcpy 占比高

可能原因：Host/Device 数据来回拷贝太多。

优化：减少传输、pinned memory、Streams 重叠、数据常驻 GPU。

##### 3. Memory-bound

现象：显存带宽高，计算利用率低。

优化：访存合并、Shared Memory、Fusion、减少不必要读写。

##### 4. Compute-bound

现象：计算单元利用率高，访存不是主要瓶颈。

优化：Tensor Core、指令级优化、减少分支、循环展开。

##### 5. Warp stall 高

需要看具体 stall 类型：

- Memory Dependency：等内存。

- Barrier：等同步。

- Not Selected：可运行 Warp 多但未被调度。

- Execution Dependency：等前序指令结果。

#### 八、优化流程

一个比较稳的流程：

1. 用 NSYS 看整体时间线，确认热点在哪里。

1. 找到最耗时 kernel。

1. 用 NCU 分析该 kernel 的访存、计算、Occupancy 和 stall。

1. 判断瓶颈是 Memory-bound、Compute-bound、同步、launch overhead 还是数据传输。

1. 做一个小改动。

1. 重新 profiling，对比指标和端到端时间。

不要只看单个指标，最终目标是端到端耗时下降。

#### 九、面试回答模板

如果问题是“NCU 和 NSYS 有什么区别”，可以这样回答：

1. NSYS 是系统级 profiler，用来看 CPU/GPU 时间线、Memcpy、Kernel、Stream 重叠和同步开销。

1. NCU 是 kernel 级 profiler，用来看单个 kernel 的访存、计算、Occupancy、Warp stall 等细节。

1. 优化通常先用 NSYS 找端到端瓶颈，再用 NCU 分析热点 kernel。

1. Occupancy 表示活跃 Warp 比例，不等于性能。

1. Memory Bandwidth 高且计算利用率低，通常是 Memory-bound；计算单元利用率高则可能是 Compute-bound。

#### 十、总结

CUDA 优化不能只凭经验。NSYS 回答“时间花在哪里”，NCU 回答“这个 kernel 为什么慢”。好的优化流程是先定位，再假设，再修改，再用指标验证。

### Nsight Compute 指标全解：从 NCU 报告定位 CUDA Kernel 优化空间

> 原文：[Nsight Compute 指标全解：从 NCU 报告定位 CUDA Kernel 优化空间](https://hd9568.github.io/blog/cuda-nsight-compute-metrics-optimization/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**面向 CUDA 算子开发，系统讲解 Nsight Compute 的采集方法、SOL、Roofline、Memory、Compute、Scheduler、Warp Stall、Occupancy、Source 与 SASS 指标，并用简单 Kernel 建立从现象到优化动作的证据链。

#### 一、NCU 到底解决什么问题

Nsight Compute，简称 NCU，是 NVIDIA 提供的 Kernel 级性能分析器。它通过 GPU 硬件计数器、软件 Patch 和静态资源信息回答下面这些问题：

```text
这个 Kernel 为什么慢？
它受计算吞吐、内存吞吐、依赖延迟、同步还是并行度限制？
最忙的是 FP32、Tensor Core、L1/TEX、L2、DRAM 还是 Shared Memory？
Warp 为什么没有发出下一条指令？
问题来自哪一行 CUDA、哪条 SASS 指令？
修改之后，减少的是工作量，还是只改变了某个百分比？
```

NCU 不擅长回答整个程序的端到端调度问题。例如：

- CPU 是否及时提交 Kernel。

- 多个 Stream 是否重叠。

- H2D、D2H、NCCL 和 Kernel 如何交错。

- 大量小 Kernel 的 Launch Gap 是否主导延迟。

- 某个 Kernel 在完整请求中的时间占比。

这些问题应该先用 Nsight Systems 分析。通常的工具链是：

```text
Nsight Systems:
  找到端到端热点和时间线问题
Nsight Compute:
  解释一个热点 Kernel 内部为什么慢
CUDA Event / Benchmark:
  在无 Profiler 干扰下确认最终延迟和吞吐
```

只有正在开发一个独立 Kernel，且已经知道它就是分析对象时，才适合直接从 NCU 开始。

#### 二、先建立正确的性能分析模型

NCU 指标很多，但 CUDA Kernel 的性能限制可以先压缩成三类。

##### 2.1 Throughput-bound：某种硬件吞吐已经接近上限

典型情况：

```text
DRAM 带宽接近可持续上限
FP32 FMA Pipe 接近可持续上限
Tensor Core Pipe 接近可持续上限
Shared Memory Pipe 或某条片上互连接近上限
```

这类 Kernel 不是“没把 GPU 用起来”，而是已经把某个资源用满。优化必须减少经过该资源的工作量，或把工作转移到其他资源。

例如 DRAM 已饱和时，单纯增加 Block 数量通常没有用。有效方向是：

- 减少 HBM 字节数。

- 提高数据复用。

- 融合 Kernel，消除中间结果。

- 使用更窄的数据类型。

- 改变 Layout，减少无效 Sector。

##### 2.2 Latency-bound：吞吐没有打满，但 Warp 经常等依赖

典型情况：

```text
Compute Throughput 不高
DRAM Throughput 也不高
Eligible Warps 很少
Long Scoreboard 或 Execution Dependency 很高
```

这不表示硬件没有瓶颈，而是单个 Warp 的依赖链太长，且并发 Warp 或指令级并行不足，无法把延迟隐藏起来。

有效方向可能是：

- 增加独立 Load 或独立累加器。

- 软件流水和 Double Buffer。

- 提前发起 Prefetch。

- 增加可驻留 Warp，但不能引入严重 Spill。

- 调整 Tile，缩短串行依赖链。

##### 2.3 Parallelism-bound：总工作量不足以覆盖整张 GPU

典型情况：

```text
Grid 只有少量 CTA
Waves Per SM 小于 1
只有部分 SM 收到工作
Compute 和 Memory 的全芯片吞吐都低
单个 CTA 内部可能并不低效
```

有效方向是增加可调度工作，而不是微调单条指令：

- 减小输出 Tile，增加 CTA 数。

- Split-K、Split-KV 或序列维切分。

- Persistent Kernel 中增加工作队列。

- 合并 Batch。

- 对短任务考虑 Kernel Fusion，但要重新评估 Grid 是否更小。

这三类问题可以同时出现。例如一个小 Grid Kernel 既有 Long Scoreboard，也只有半个 Wave。必须先估计每类问题的性能上限，优先处理收益最大的限制。

#### 三、准备可信的 Profile 环境

##### 3.1 先保证结果正确

错误的越界访问、Race 或未初始化数据可能在普通运行时没有立即暴露，但在 Replay 时表现完全不同。建议在 Profile 前先运行：

```bash
compute-sanitizer --tool memcheck ./ncu_demo
compute-sanitizer --tool racecheck ./ncu_demo
```

`racecheck` 主要检查 Shared Memory Hazard，不会替代所有并发正确性验证。

##### 3.2 使用 Release 优化并保留行号

推荐编译方式：

```bash
nvcc -O3 -lineinfo -Xptxas=-v -arch=sm_90 \
  ncu_demo.cu -o ncu_demo
```

参数含义：

| 参数 | 作用 |
| --- | --- |
| `-O3` | 使用优化后的 Device Code |
| `-lineinfo` | 保留 CUDA 源码与 SASS 的行号映射 |
| `-Xptxas=-v` | 输出寄存器、Shared Memory、Spill 等编译信息 |
| `-arch=sm_XX` | 为实际目标架构生成代码 |

不要用 `-G` 生成 Device Debug Code 后分析性能。`-G` 会改变优化、寄存器分配和指令序列，得到的不是发布版本 Kernel。

##### 3.3 Profile 之前先 Warmup

首次运行可能包含：

- CUDA Context 初始化。

- Module Lazy Loading。

- PTX JIT。

- 库内部 Heuristic 或 Autotune。

- 内存页建立。

- GPU 从 Idle P-State 升频。

Benchmark 应 Warmup 后再计时。NCU 则可以通过 Kernel 过滤和 `--launch-skip` 跳过 Warmup。

##### 3.4 固定输入、Shape 和运行环境

至少记录：

```text
GPU 型号与 Compute Capability
Driver、CUDA、NCU 版本
Kernel Shape、Dtype、Layout
Grid、Block、Dynamic Shared Memory
编译选项
时钟与功耗状态
MIG 配置
NCU 的 Cache Control、Clock Control 和 Replay Mode
```

动态负载、原子竞争和工作队列会让不同运行的指令路径不同。若两次报告不是同一工作负载，差异视图没有意义。

##### 3.5 不要把 NCU 中的耗时当最终性能

NCU 可能：

- 序列化 Kernel Launch。

- 多次 Replay 同一个 Kernel。

- 保存并恢复 Kernel 使用的内存。

- Flush Cache。

- Patch 指令或开启采样。

- 控制 GPU Clock。

因此最终性能必须在无 NCU 的条件下，用 CUDA Event 或框架 Benchmark 重新测量。NCU 的 Duration 适合在相同采集配置下辅助对比，不应直接替代生产延迟。

#### 四、NCU 命令行与 Kernel 过滤

##### 4.1 先查看当前版本支持什么

不同 NCU 版本和 GPU 架构可用的 Section、Metric 不完全相同。不要从旧文章复制一长串 Metric 名称后直接使用。

```bash
ncu --version
ncu --list-sets
ncu --list-sections
ncu --query-metrics
```

查询某个 Base Metric 支持哪些后缀：

```bash
ncu --query-metrics-mode suffix \
  --metrics sm__throughput
```

##### 4.2 从轻量采集开始

不指定 `--set`、`--section` 或 `--metrics` 时，NCU 默认采集 `basic` Set：

```bash
ncu -o report_basic ./ncu_demo
```

更常用的第一轮：

```bash
ncu --set basic \
  --kernel-name regex:copy_ \
  --launch-skip 10 \
  --launch-count 1 \
  -o report_basic \
  ./ncu_demo
```

含义：

- `--kernel-name regex:copy_`：只匹配名字包含 `copy_` 的 Kernel。

- `--launch-skip 10`：在匹配到的 Launch 中跳过前 10 次。

- `--launch-count 1`：只采集随后 1 次。

- `-o report_basic`：保存为 `.ncu-rep` 报告。

Kernel 是 C++ 模板时，完整 Demangled Name 可能很长。先运行轻量采集确认名称，再写更精确的 Regex。

##### 4.3 按问题选择 Section

比一上来 `--set full` 更高效的做法是只采集当前需要的 Section：

```bash
ncu \
  --section SpeedOfLight \
  --section LaunchStats \
  --section Occupancy \
  --section SchedulerStats \
  --section WarpStateStats \
  --section MemoryWorkloadAnalysis \
  --section ComputeWorkloadAnalysis \
  --kernel-name regex:copy_ \
  --launch-skip 10 \
  --launch-count 1 \
  -o report_detail \
  ./ncu_demo
```

需要完整探索时再使用：

```bash
ncu --set full \
  --kernel-name regex:copy_ \
  --launch-count 1 \
  -o report_full \
  ./ncu_demo
```

`full` 可能需要很多 Replay Pass。Kernel 越长、写入内存越多，采集成本越高。

##### 4.4 只采集少量原始 Metric

自动化回归更适合指定有限 Metric：

```bash
ncu \
  --metrics "gpu__time_duration.sum,sm__throughput.avg.pct_of_peak_sustained_elapsed,dram__throughput.avg.pct_of_peak_sustained_elapsed" \
  --kernel-name regex:copy_ \
  --launch-count 1 \
  ./ncu_demo
```

这些名称只是常见示例。应使用目标机器上的 `--query-metrics` 验证。

##### 4.5 多进程、NVTX 与程序内控制

Python 框架或 Worker 子进程需要：

```bash
ncu --target-processes all [其他参数] python workload.py
```

复杂应用可以用 NVTX 标记目标范围，再启用 NVTX Filter：

```bash
ncu --nvtx --nvtx-include "profile/" [其他参数] ./app
```

也可以在程序中用 `cudaProfilerStart()` 和 `cudaProfilerStop()` 包围目标区间，并使用：

```bash
ncu --profile-from-start off [其他参数] ./app
```

目标是尽量少采集无关 Launch。减少 Profile 范围通常比增加机器资源更有效。

#### 五、理解 Set、Section、Metric 与 Replay

##### 5.1 Set 是预定义的采集规模

Set 是一组 Section 的集合：

```text
basic:
  采集较少，适合第一轮分类
full:
  覆盖更多分析维度，采集更慢
```

Set 不是“精度等级”。`basic` 中已有的 Metric 不会因为切换到 `full` 就自动更准确，`full` 只是收集更多证据。

##### 5.2 Section 是围绕一个问题组织的指标和规则

常用 Section：

| Section | 主要回答的问题 |
| --- | --- |
| `SpeedOfLight` | Compute 和 Memory 哪个大类更接近上限 |
| `SpeedOfLight_RooflineChart` | FLOPs、字节数和算术强度位于哪个 Roof 下 |
| `LaunchStats` | Grid、Block、寄存器和 Shared 配置是什么 |
| `Occupancy` | 哪种资源限制 CTA/Warp 驻留 |
| `SchedulerStats` | Scheduler 每周期有多少 Active、Eligible、Issued Warp |
| `WarpStateStats` | Warp 的周期主要消耗在哪些状态 |
| `MemoryWorkloadAnalysis` | L1/TEX、L2、DRAM、Shared 的请求和吞吐如何 |
| `ComputeWorkloadAnalysis` | 哪条执行管线最忙，指令吞吐如何 |
| `InstructionStats` | SASS 指令混合与执行数量是什么 |
| `SourceCounters` | 哪行源码对应高 Stall、低线程利用率或异常访问 |

Section 内除了 Metric，还可以包含 NVIDIA 提供的 Rule。Rule 会根据指标给出提示，但它是启发式规则，不是性能问题的证明。

##### 5.3 为什么需要 Replay

GPU 同时可采集的硬件计数器有限，而一份完整报告需要很多 Metric。NCU 会将 Metric 分成多个 Pass，并多次执行目标 Kernel：

```text
Pass 1: 采集一组 SM Counter
Pass 2: 采集一组 L1/TEX Counter
Pass 3: 采集一组 L2/DRAM Counter
...
```

Kernel Replay 通常会保存 Kernel 写入的内存，并在后续 Pass 前恢复，以尽量让每次执行看到相同状态。

这带来几个重要后果：

1. 原子竞争和动态调度可能无法完全复现。

1. Kernel 写入的内存范围越大，保存与恢复成本越高。

1. Cache Flush 策略会影响 Hit Rate。

1. Profile 期间的 Kernel Duration 不等于无 Profiler Duration。

1. 不同 Pass 的 Counter 来自不同执行，不应分析一次运行内的瞬时相关性。

当 Kernel Replay 不适合时，可以了解 Application Replay 或 Range Replay。它们通过重新执行整个应用或范围收集不同 Pass，但要求应用具有足够的可重现性。

##### 5.4 Cache Control 与真实业务缓存

为了让 Replay Pass 可比较，NCU 可以在 Pass 之间控制 Cache。默认策略可能与生产中的 Warm Cache 不同。

例如某个权重常驻 L2 的 Decode Kernel，如果每个 Pass 都 Flush Cache，报告会更接近 Cold Cache；如果关闭 Cache Flush，后续 Pass 又可能比第一次更热。

正确做法不是固定选择某一个模式，而是：

```text
1. 明确生产环境是 Cold、Warm 还是多租户抖动。
2. 所有候选实现使用相同 NCU Cache 配置。
3. 用无 Profiler 的端到端 Benchmark 验证。
4. 必要时分别报告 Cold Cache 和 Warm Cache。
```

#### 六、读懂 Metric 名称和归一化方式

PerfWorks Metric 常见形式：

```text
sm__throughput.avg.pct_of_peak_sustained_elapsed
```

可以拆成：

```text
sm__throughput
  sm              -> 计数单元或硬件单元
  throughput      -> Base Metric
.avg
  跨实例聚合方式
.pct_of_peak_sustained_elapsed
  相对可持续峰值的百分比，以 Kernel Elapsed Cycles 归一化
```

##### 6.1 常见硬件单元前缀

| 前缀 | 大致对应 |
| --- | --- |
| `gpu__` | 整个 GPU 或 Kernel 级信息 |
| `sm__` | Streaming Multiprocessor |
| `smsp__` | SM Subpartition，通常与 Warp Scheduler 侧指标相关 |
| `l1tex__` | L1/TEX 单元，处理 Global、Local、Texture、Shared 等请求 |
| `lts__` | L2 Cache Slice |
| `dram__` | Device Memory / HBM / GDDR |

不能仅凭前缀推导全部含义，最终以 `--query-metrics` 描述和官方文档为准。

##### 6.2 常见 Rollup 后缀

| 后缀 | 含义 |
| --- | --- |
| `.sum` | 所有实例或周期累计 |
| `.avg` | 实例平均 |
| `.min` / `.max` | 实例最小值或最大值 |
| `.per_second` | 换算为每秒速率 |
| `.per_cycle_active` | 以单元 Active Cycle 归一化 |
| `.per_cycle_elapsed` | 以整个 Elapsed Cycle 归一化 |
| `.pct_of_peak_sustained_active` | Active 期间相对可持续峰值 |
| `.pct_of_peak_sustained_elapsed` | 整个 Kernel 期间相对可持续峰值 |
| `.per_warp_active` | 以 Active Warp 归一化 |

`active` 与 `elapsed` 不能混着比较。某个单元只在一小段时间内工作时：

```text
pct_of_peak_sustained_active 可能很高
pct_of_peak_sustained_elapsed 仍然很低
```

前者表示它一旦工作就很忙，后者表示它对整个 Kernel 时间的覆盖率有限。

##### 6.3 Peak Sustained 不等于规格表峰值

`pct_of_peak_sustained_*` 的分母是 NCU 对该硬件单元定义的可持续峰值，不一定等于产品规格表中通过频率和 Core 数推导的 Marketing Peak。

同样，`Compute Throughput = 80%` 也不表示“80% 的 CUDA Core 都在工作”。它通常是相关计算子单元归一化吞吐中的代表值或较高值。必须展开 Compute Workload 才知道是 FP32、Tensor、ALU、SFU 还是其他 Pipe 在贡献。

#### 七、推荐的 NCU 诊断顺序

不要从几百个 Metric 中随机挑高值。推荐按以下顺序收敛。

##### 第一步：确认 Kernel 值得优化

用 NSYS 或端到端统计确认：

```text
Kernel 占总延迟多少？
调用频率多高？
优化 100% 后，端到端上限是多少？
```

Amdahl 定律：

```text
端到端加速比 =
1 / ((1 - p) + p / s)
p: 目标 Kernel 的时间占比
s: Kernel 自身加速比
```

##### 第二步：看 Duration、Grid 与 Waves

先判断：

- Kernel 是否短到测量噪声很大。

- Grid 是否覆盖全部 SM。

- 是否存在明显 Tail Wave。

- 每个 Block 使用多少寄存器和 Shared Memory。

##### 第三步：看 Speed Of Light 和 Roofline

粗分：

```text
Compute Throughput 高 -> 深挖 Compute Pipe
Memory Throughput 高  -> 深挖内存层级
两者都低             -> 深挖并行度、Scheduler、Stall、同步
```

##### 第四步：看 Scheduler 和 Warp Stall

确认是：

- 没有足够 Active Warp。

- 有 Active Warp，但没有 Eligible Warp。

- Eligible Warp 足够，某条 Pipe 已满。

##### 第五步：进入具体子系统

```text
Memory:
  Request -> Sector -> Wavefront -> L1 -> L2 -> DRAM
Compute:
  SASS Mix -> Pipe Utilization -> Dependency -> Thread Predication
Synchronization:
  Barrier Stall -> CTA 内工作量差异 -> Source Line
```

##### 第六步：定位 Source/SASS 并提出单变量修改

每次修改都应该有预测：

```text
改动:
  将跨步 Global Load 改成连续 Load
预期:
  Global Load Sectors/Request 下降
  DRAM Bytes 下降
  Long Scoreboard 下降或更容易隐藏
  Duration 下降
```

如果指标按预期变化但 Duration 不变，说明修复的不是当前最大瓶颈，或者新的瓶颈已经接管。

#### 八、Speed Of Light：先看哪个硬件子系统最忙

Speed Of Light，简称 SOL，提供 Kernel 的高层吞吐概览。

常见顶部指标：

```text
Compute Throughput
Memory Throughput
Duration
Elapsed Cycles
SM Frequency
DRAM Frequency
```

##### 8.1 四种基本组合

| Compute | Memory | 初步判断 |
| --- | --- | --- |
| 高 | 低 | 某条计算 Pipe 可能是吞吐瓶颈 |
| 低 | 高 | 某级内存或数据通路可能是吞吐瓶颈 |
| 高 | 高 | 计算与搬运较平衡，需判断谁在关键路径 |
| 低 | 低 | 并行度不足、依赖延迟、同步或工作量太小 |

这里的“高”没有跨架构固定阈值。对于一个成熟 GEMM，80% 可能仍有价值；对于包含多种阶段的融合 Kernel，某个单元达到 50% 已可能是关键限制。

##### 8.2 Memory Throughput 高不等于 DRAM 高

Memory Throughput 是内存系统高层汇总。它可能由以下任一单元主导：

```text
L1/TEX
L2
DRAM
Shared Memory 路径
片上互连
```

因此：

```text
Memory Throughput = 90%
```

不能直接写成：

```text
HBM 带宽已经达到 90%
```

必须在 Memory Workload Analysis 中展开，看 `dram__throughput`、L2、L1/TEX 和 Shared 具体哪一级高。

##### 8.3 Compute Throughput 高不等于 FLOPs 高

地址计算、整数 ALU、类型转换、特殊函数和 Tensor Core 都属于计算资源的一部分。一个只有少量浮点运算的索引 Kernel，也可能因为整数 Pipe 或特殊指令达到较高 Compute Throughput。

下一步应看：

- Compute Workload Analysis 的各 Pipe Utilization。

- Instruction Statistics 的 SASS Mix。

- Source Page 上热点指令。

##### 8.4 两者都低时不要立刻判定“还有很大优化空间”

小 Grid Kernel 只占用少量 SM 时，全芯片 SOL 自然很低。单个 SM 上可能已经无法更快。

依赖链很长时，执行单元大部分周期没有指令输入，SOL 也会低。此时增加算术强度未必有效，应该先解决 Eligible Warp 和 ILP。

所以 SOL 是入口，不是最终诊断。

#### 九、Roofline：计算上限、带宽上限与算术强度

Roofline 用两个硬件上限约束可达到的计算性能：

```text
P <= P_compute_peak
P <= Bandwidth_peak * Arithmetic_Intensity
```

其中：

```text
Arithmetic Intensity = FLOPs / Bytes
```

于是：

```text
P_attainable = min(
    P_compute_peak,
    Bandwidth_peak * Arithmetic_Intensity
)
```

##### 9.1 Vector Add 为什么天然偏 Memory-bound

对 FP32 Vector Add：

```cpp
c[i] = a[i] + b[i];
```

理想情况下每个元素：

```text
读取 a: 4 Bytes
读取 b: 4 Bytes
写入 c: 4 Bytes
计算:   1 FLOP
Arithmetic Intensity = 1 / 12 FLOP/Byte
```

它几乎不可能靠提高 FP32 算力显著加速。正确方向是减少字节数或把 Add 融合进前后 Kernel。

如果表达式是：

```cpp
y[i] = a * x[i] + y[i];
```

FMA 通常按 2 FLOPs 计，字节数仍约为 12：

```text
Arithmetic Intensity = 2 / 12 = 1 / 6 FLOP/Byte
```

仍然是低算术强度。

##### 9.2 GEMM 为什么可以变成 Compute-bound

对 `M x K` 与 `K x N` GEMM：

```text
FLOPs ~= 2 * M * N * K
```

通过 Tiling，每个加载到 Shared Memory 或寄存器的数据参与多次 FMA，实际算术强度可以显著提高。当点跨过 Ridge Point 后，性能上限从带宽斜线转为计算横线。

##### 9.3 Roofline 中的 Bytes 必须指定层级

同一个 Kernel 在不同内存层级有不同 Arithmetic Intensity：

```text
AI_DRAM = FLOPs / DRAM Bytes
AI_L2   = FLOPs / L2 Bytes
AI_L1   = FLOPs / L1 Bytes
```

一个 GEMM 可能对 DRAM 有很高复用，但在 Shared Memory 上产生巨大流量。只看 DRAM Roofline 会漏掉片上瓶颈。

##### 9.4 点落在 Roof 下方不一定是“带宽没优化好”

Roofline 描述吞吐上限，但不自动解释为什么没有接近上限。点远离所有 Roof 可能来自：

- Grid 太小。

- Warp Divergence。

- Barrier。

- Long Dependency Chain。

- 非浮点指令主导。

- Load Imbalance。

- Tail Effect。

- 指令发射或内存请求队列受限。

Roofline 适合做瓶颈分类和性能上限估算，不能替代 Scheduler 与 Source 分析。

#### 十、Launch Statistics：Grid、Block 与 Wave Quantization

Launch Statistics 是最容易被忽略、但成本最低的一组信息。

重点查看：

```text
Grid Size
Block Size
Registers Per Thread
Static Shared Memory Per Block
Dynamic Shared Memory Per Block
Threads
Waves Per SM
```

##### 10.1 Waves Per SM

假设：

```text
GPU 有 S 个 SM
每个 SM 最多同时驻留 B 个目标 CTA
Grid 一共有 G 个 CTA
```

一轮完整 Wave 最多容纳：

```text
S * B 个 CTA
```

近似有：

```text
Waves Per SM = G / (S * B)
```

解释：

```text
Waves < 1:
  Grid 不足以填满所有并发槽位
Waves ~= 整数:
  CTA 分布通常较完整
Waves = k + 很小的小数:
  最后一轮只有少量 CTA，可能出现明显 Tail
```

例如前 4 个 Wave 都填满 GPU，最后只剩 5 个 CTA。Kernel 末尾大部分 SM 会空闲，但 Duration 要等这 5 个 CTA 完成。

##### 10.2 Wave Quantization 的优化方向

- 改变 Tile Size，增加或调整 CTA 数量。

- 对归约维使用 Split-K。

- 对 Attention KV 维使用 Split-KV。

- 让不同 CTA 工作量更均衡。

- 使用 Work Queue 或 Persistent Scheduling。

- 合并多个小 Batch。

不要为了让 Wave 数好看而盲目减小 Tile。Tile 变小会降低数据复用、增加边界处理和 Epilogue 数量。应比较总 Duration。

##### 10.3 Block Size 不是越大越好

大 Block 可能：

- 减少 Grid CTA 数。

- 增加每 CTA 寄存器和 Shared Memory。

- 降低每 SM 可驻留 CTA 数。

- 增加 Barrier 等待范围。

小 Block 也可能：

- 每个 CTA 工作太少。

- 增加调度与边界开销。

- 降低跨线程数据复用。

Block Size 必须和 Tile、资源占用、Scheduler 及 Memory Transaction 一起看。

##### 10.4 检查 SM 与 L2 Slice 间的工作量不均

全芯片平均值可能掩盖实例间不均衡。例如一部分 SM 工作很久，其他 SM 很早结束：

```text
SM Active Cycles Max 明显高于 Avg
Achieved Occupancy 低于 Theoretical Occupancy
末尾只有少量 SM Active
```

这通常来自：

- CTA 工作量与数据内容相关。

- 不同 Tile 的稀疏度、Mask 或有效长度不同。

- Persistent Work Queue 分配不均。

- 最后一个 Partial Wave。

L2 Slice 也可能不均衡。若地址分布、Partition Camping 或热点数据让少量 Slice 承担大部分 Sector，平均 L2 Throughput 看起来不高，但最忙 Slice 已接近上限。

应比较相关实例的 `min/avg/max`，并在 NCU GUI 中查看 Workload Distribution 或吞吐分解。优化方向包括重新编号 Tile、交错地址、动态任务队列和更细粒度任务切分。

#### 十一、Occupancy：理论驻留能力不等于执行效率

Occupancy 定义为：

```text
Occupancy =
Active Warps Per SM / Maximum Warps Per SM
```

NCU 通常同时给出：

```text
Theoretical Occupancy
Achieved Occupancy
```

##### 11.1 Theoretical Occupancy 由什么限制

每个 SM 能驻留多少 CTA，受多个上限共同约束：

```text
Block Limit SM
Block Limit Warps
Block Limit Registers
Block Limit Shared Memory
Block Limit Blocks
```

最终：

```text
Resident Blocks Per SM =
min(各资源允许的 Block 数)
```

注意资源按硬件粒度分配。寄存器和 Shared Memory 的实际占用可能向上取整，不是简单连续除法。

##### 11.2 Achieved 低于 Theoretical 说明什么

常见原因：

- Grid 太小，无法填满所有 SM。

- 最后一个 Partial Wave 占比大。

- CTA 工作量不均衡。

- 部分 Warp 提前退出。

- Kernel 运行太短，时间平均受到启动和收尾阶段影响。

因此很大的 Theoretical/Achieved Gap 通常应该和 Waves、SM 间工作分布一起看。

##### 11.3 Occupancy 高为什么仍然可能慢

高 Occupancy 只表示有很多 Warp 驻留，不表示这些 Warp：

- 正在执行指令。

- 有合并的内存访问。

- 没有 Bank Conflict。

- 没有 Barrier。

- 使用了 Tensor Core。

大量 Warp 可以同时卡在同一条长延迟依赖上。

##### 11.4 Occupancy 低什么时候值得优化

同时满足下列证据时，Occupancy 更可能是真问题：

```text
Achieved Occupancy 低
Eligible Warps Per Scheduler 低
No Eligible 周期多
Long Scoreboard 或 Execution Dependency 高
目标 Pipe 和 Memory Throughput 都未饱和
```

此时可以尝试：

- 降低每线程寄存器，但检查 Spill。

- 减少每 CTA Shared Memory。

- 调整 Block Size。

- 缩小 Tile。

- 减少不必要的循环展开。

如果低 Occupancy 下 Tensor Core 已接近峰值，强行提高 Occupancy 通常没有收益，甚至会破坏寄存器复用。

#### 十二、Scheduler Statistics：每个周期为什么发不出指令

Scheduler Statistics 是连接“硬件不忙”和“Warp 在等什么”的关键。

对每个 Warp Scheduler，可以把 Warp 分成：

```text
Theoretical Warps:
  Launch 配置理论上允许分配给 Scheduler 的 Warp
Active Warps:
  已驻留、尚未完成的 Warp
Eligible Warps:
  下一条指令已经就绪，可以发射的 Warp
Issued Warps:
  本周期实际被 Scheduler 选中并发射指令的 Warp
```

关系近似为：

```text
Issued <= Eligible <= Active <= Theoretical
```

##### 12.1 Active 少

说明可供 Scheduler 隐藏延迟的 Warp 不足。继续检查：

- Occupancy 是否被寄存器或 Shared Memory 限制。

- Grid/Waves 是否过小。

- Block 是否只有很少 Warp。

##### 12.2 Active 多，但 Eligible 少

这是典型的延迟隐藏失败：

```text
有很多 Warp 驻留
但大多数 Warp 都在等待依赖、内存或同步
```

下一步看 Warp Stall：

- Long Scoreboard：常见于 Global/Local Load 结果未返回。

- Short Scoreboard：常见于 Shared Memory 或其他较短依赖。

- Execution Dependency：ALU 结果依赖链。

- Barrier：CTA 同步等待。

- Throttle：请求队列或 Pipe 无法继续接收指令。

##### 12.3 Eligible 多，但 Issued 接近发射上限

Scheduler 手里有足够工作，也在持续发射。这往往表示：

- 某条执行 Pipe 正在被高效使用。

- `Not Selected` Stall 可能升高，因为多个 Eligible Warp 竞争发射槽。

此时 `Not Selected` 高通常不是坏事。它说明 Warp 已经准备好，只是本周期另一个 Warp 被选中。应检查 Compute 或 Memory Pipe 是否接近吞吐上限，而不是试图“消灭 Not Selected”。

##### 12.4 Issued 很低且 No Eligible 很高

说明大量发射槽浪费。优化重点是产生更多 Eligible Warp：

- 提高 TLP：更多可驻留 Warp。

- 提高 ILP：同一 Warp 中更多独立指令。

- 缩短依赖链。

- 更早发起异步拷贝或 Load。

- 减少 Barrier 和负载不均。

#### 十三、Warp Stall：每种停顿究竟说明什么

不同版本的指标名称可能带有 `warp_issue_stalled`、`warps_issue_stalled` 或派生 Ratio。应优先在 Warp State Statistics 和 Source Counters 中查看，再查询当前版本的完整 Metric 名。

##### 13.1 先理解 Stall 百分比的分母

Warp Stall 指标通常描述 Active Warp 在采样或统计周期中的状态分布。它不等于：

```text
Kernel 有 40% 的墙钟时间完全浪费在 Long Scoreboard
```

多个 Warp 可以在同一周期处于不同状态，而 Scheduler 仍可能从另一个 Eligible Warp 发出指令。

只有 Scheduler 显示 Eligible Warp 不足、Issue Slot 经常空闲时，高 Stall 才直接指向吞吐损失。

##### 13.2 Long Scoreboard

含义：

```text
Warp 的下一条指令依赖一个尚未完成的 L1TEX 相关操作
```

常见来源：

- Global Memory Load。

- Local Memory Load，也就是可能的 Spill。

- Texture 或 Surface Load。

- Cache Miss 后等待 L2/DRAM。

- 不规则 Pointer Chasing。

优化方向：

- 合并访问，减少 Sector。

- 提高 L1/L2 数据复用。

- 减少 Spill。

- Prefetch 和软件流水。

- 增加独立 Load 或独立计算。

- 增加足够的并发 Warp。

- 对 Pointer Chasing 改变数据结构。

Long Scoreboard 高不自动等于 DRAM 带宽瓶颈。它更接近“等待 Load 延迟”。DRAM Throughput 可能很低，因为请求太少或访问过于串行。

##### 13.3 Short Scoreboard

通常表示等待较短的 Scoreboard 依赖，常见来源是 MIO 路径上的 Shared Memory 操作。

重点检查：

- Shared Load/Store Bank Conflict。

- Shared Memory 数据依赖距离太短。

- `ldmatrix` 或片上数据搬运后的立即消费。

- Shared Memory Pipe 是否饱和。

优化方向：

- Padding 或 Swizzle 消除 Bank Conflict。

- 在 Load 和 Use 之间插入独立计算。

- Double Buffer。

- 调整 Warp 的数据映射。

##### 13.4 Barrier

Warp 正在等待 CTA 级 Barrier，例如 `__syncthreads()` 或相关同步原语。

高 Barrier Stall 可能来自：

- Barrier 太频繁。

- Barrier 前不同 Warp 工作量不一致。

- 某些 Warp 的 Global Load 更慢。

- CTA 内 Producer/Consumer 分工不平衡。

- Tile 边界导致部分 Warp 走更长路径。

优化方向：

- 减少不必要的 Block 级同步。

- 能用 `__syncwarp()` 时不要扩大到整个 CTA。

- 平衡 Barrier 前工作量。

- 使用异步 Pipeline 和更细粒度 Barrier。

- 调整 Warp Specialization 的 Producer/Consumer 比例。

不要直接删除 Barrier。必须先证明不存在跨 Warp 数据依赖。

##### 13.5 MIO Throttle

MIO 指令队列或相关 Pipe 无法继续接收指令。常见相关操作包括：

- Shared Memory。

- 特殊数学或其他通过 MIO 的操作。

- 部分数据移动指令。

优化方向依赖 Source/SASS：

- 减少 Shared 指令数。

- 提高每次 Shared Load 的有效数据量。

- 改用 Shuffle 传递 Warp 内数据。

- 将某些索引或转换与其他 Pipe 交错。

##### 13.6 LG Throttle

Local/Global Memory 指令发射过密，相关队列出现背压。

常见原因：

- 每做很少计算就发很多 Global Load/Store。

- Spill 产生大量 Local Load/Store。

- 非合并访问让请求膨胀。

优化方向：

- 减少内存指令数量。

- 使用向量化 Load/Store，但先保证对齐与连续。

- 提高寄存器或 Shared Memory 复用。

- 消除 Spill。

- 融合相邻访问。

##### 13.7 Math Pipe Throttle

目标数学 Pipe 已经接近可接收指令的上限。它通常是吞吐瓶颈证据，而不是调度故障。

下一步：

- 找到具体是 FP32、FP64、Tensor、ALU、SFU 还是其他 Pipe。

- 减少该类指令。

- 使用更合适的数据类型或 Tensor Core。

- 将部分工作转移到其他 Pipe。

- 用近似数学指令时验证数值误差。

##### 13.8 Wait / Execution Dependency

不同 NCU 版本可能将这类状态显示为 `Wait` 或 Execution Dependency。Warp 在等待前序固定延迟指令的结果，典型是串行累加：

```cpp
for (int k = 0; k < K; ++k) {
    sum = fmaf(a[k], b[k], sum);
}
```

每次 FMA 都依赖上一次 `sum`。可以使用多个独立累加器：

```cpp
float sum0 = 0.0f;
float sum1 = 0.0f;
for (int k = 0; k < K; k += 2) {
    sum0 = fmaf(a[k],     b[k],     sum0);
    sum1 = fmaf(a[k + 1], b[k + 1], sum1);
}
float sum = sum0 + sum1;
```

代价是更多寄存器。需要同时看 Occupancy 与 Spill。

##### 13.9 Not Selected

Warp 是 Eligible 状态，但 Scheduler 选择了另一个 Warp。

高 `Not Selected` 往往表示：

```text
Scheduler 有足够的 Ready Warp
延迟隐藏较好
当前发射槽或执行 Pipe 已被其他 Warp 使用
```

它通常不需要单独优化。只有在某类优先级、调度公平性或特定 Warp Specialization 问题中，才需要进一步分析。

##### 13.10 Branch Resolving、No Instruction 与其他 Stall

- `Branch Resolving`：等待分支目标或分支结果解析。

- `No Instruction`：下一条指令尚未可获取，可能涉及指令 Cache 或 Fetch。

- `Membar`：等待内存屏障要求满足。

- `Drain`：Warp 收尾时等待未完成操作排空，常与工作不均和 Store 有关。

- `Sleep`：执行显式 Sleep/Nanosleep。

这些状态通常需要 Source/SASS 和算法上下文。不要只根据名字做修改。

#### 十四、Memory Workload：从请求追到 DRAM

内存分析最重要的不是只看一个带宽，而是沿请求路径追踪：

```text
Thread Address
  -> Warp Memory Instruction
  -> Request
  -> 32-Byte Sectors
  -> L1/TEX Wavefront
  -> L2 Sectors
  -> DRAM Bytes
```

##### 14.1 Request

一个 Warp 执行一条内存指令时，会形成一个或多个请求。Request 不是每线程一个。

##### 14.2 Sector

现代 NVIDIA Cache 统计常以 32-Byte Sector 为粒度。一个 Warp 的地址覆盖多少 Sector，直接反映访问是否连续、对齐和稀疏。

以 32 个线程各读一个 FP32 为例：

```text
有效数据 = 32 * 4 B = 128 B
理想且对齐时 = 4 个 32 B Sector
```

如果地址跨步很大，最坏可能接近：

```text
32 个线程触及 32 个不同 Sector
```

实际传输数据相对有效数据膨胀约 8 倍。

##### 14.3 Wavefront

L1TEX 每个周期能处理的 Sector 数有限。一个 Request 若包含过多 Sector，可能拆成多个 Wavefront。

因此：

```text
Request 数不变
Sector 数增加
Wavefront 数增加
L1TEX 占用时间和下游流量增加
```

Shared Memory Bank Conflict 也可能表现为 Excessive Wavefront，因为一次访问需要被序列化成多个阶段。

##### 14.4 Bytes 与 Throughput

建议同时看：

```text
DRAM Read/Write Bytes
L2 Read/Write Sectors 或 Bytes
L1/TEX Sectors
实际 GB/s
pct_of_peak_sustained
```

Bytes 回答“做了多少工作”，Throughput 回答“单位时间做得多快”。优化后 Duration 下降时，GB/s 可能上升，即使 Bytes 已下降；这不是矛盾。

##### 14.5 判断 Memory-bound 的完整证据

比较可信的证据链：

```text
DRAM Throughput 接近可持续峰值
DRAM Bytes 与算法预期一致或偏高
Roofline 位于 DRAM Bandwidth Roof 附近
Compute Pipe 未成为更强限制
增加计算并行度不能提升性能
减少 DRAM Bytes 后 Duration 近似同比下降
```

只有 `Long Scoreboard` 高，不足以证明 Bandwidth-bound。

#### 十五、Global Memory：用 Sector 判断合并访问

下面用两个简单 Kernel 对比连续访问和跨步访问。

##### 15.1 可编译示例

```cpp
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#define CHECK_CUDA(call)                                                   \
    do {                                                                   \
        cudaError_t err = (call);                                          \
        if (err != cudaSuccess) {                                          \
            std::fprintf(stderr, "%s:%d CUDA error: %s\n",                 \
                         __FILE__, __LINE__, cudaGetErrorString(err));      \
            std::exit(EXIT_FAILURE);                                       \
        }                                                                  \
    } while (0)
__global__ void copy_contiguous(
    const float* __restrict__ input,
    float* __restrict__ output,
    int width,
    int height) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row < height && col < width) {
        int index = row * width + col;
        output[index] = input[index];
    }
}
__global__ void copy_strided_read(
    const float* __restrict__ input,
    float* __restrict__ output,
    int width,
    int height) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row < height && col < width) {
        int output_index = row * width + col;
        int input_index = col * height + row;
        output[output_index] = input[input_index];
    }
}
int main(int argc, char** argv) {
    constexpr int width = 4096;
    constexpr int height = 4096;
    constexpr int warmup = 10;
    const size_t bytes = static_cast<size_t>(width) * height * sizeof(float);
    float* input = nullptr;
    float* output = nullptr;
    CHECK_CUDA(cudaMalloc(&input, bytes));
    CHECK_CUDA(cudaMalloc(&output, bytes));
    CHECK_CUDA(cudaMemset(input, 0, bytes));
    dim3 block(32, 8);
    dim3 grid((width + block.x - 1) / block.x,
              (height + block.y - 1) / block.y);
    bool strided = argc > 1 && std::string(argv[1]) == "strided";
    for (int i = 0; i < warmup + 1; ++i) {
        if (strided) {
            copy_strided_read<<<grid, block>>>(
                input, output, width, height);
        } else {
            copy_contiguous<<<grid, block>>>(
                input, output, width, height);
        }
    }
    CHECK_CUDA(cudaGetLastError());
    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaFree(output));
    CHECK_CUDA(cudaFree(input));
    return 0;
}
```

编译：

```bash
nvcc -O3 -lineinfo -Xptxas=-v -arch=sm_XX \
  ncu_demo.cu -o ncu_demo
```

采集连续版本：

```bash
ncu \
  --section SpeedOfLight \
  --section MemoryWorkloadAnalysis \
  --section SchedulerStats \
  --section WarpStateStats \
  --launch-skip 10 \
  --launch-count 1 \
  -o contiguous \
  ./ncu_demo
```

采集跨步版本：

```bash
ncu \
  --section SpeedOfLight \
  --section MemoryWorkloadAnalysis \
  --section SchedulerStats \
  --section WarpStateStats \
  --launch-skip 10 \
  --launch-count 1 \
  -o strided \
  ./ncu_demo strided
```

##### 15.2 为什么连续版本更容易合并

`block.x = 32`，同一 Warp 的 `threadIdx.x` 连续：

```text
input[index + 0]
input[index + 1]
...
input[index + 31]
```

一个 Warp 读取 128 Bytes 连续且对齐的数据，理想情况下覆盖 4 个 32-Byte Sector。

跨步版本中：

```text
input[row + 0  * height]
input[row + 1  * height]
...
input[row + 31 * height]
```

相邻 Lane 相差 `height * sizeof(float)` Bytes。每个 Lane 几乎都会落到不同 Sector。

##### 15.3 预期看到的指标方向

| 指标 | 连续访问 | 跨步读取 |
| --- | --- | --- |
| Global Load Sectors/Request | 接近理想值 | 显著增加 |
| L1/TEX Wavefront | 少 | 多 |
| DRAM/L2 Bytes | 接近有效字节 | 可能膨胀 |
| Long Scoreboard | 较低或可隐藏 | 更容易升高 |
| Duration | 较短 | 较长 |

具体 Hit Rate 取决于 Cache 容量、架构和 Replay 策略，不应预设固定值。

##### 15.4 常用底层 Metric

不同版本名称可能变化，常见 Base Metric 方向包括：

```text
l1tex__t_requests_*_mem_global_op_ld
l1tex__t_sectors_*_mem_global_op_ld
l1tex__t_requests_*_mem_global_op_st
l1tex__t_sectors_*_mem_global_op_st
dram__bytes_read
dram__bytes_write
```

核心派生量：

```text
Load Sectors Per Request =
Global Load Sectors / Global Load Requests
```

理想值取决于：

- 每 Lane 访问的字节数。

- Active Lane 数量。

- 地址对齐。

- 是否跨越 Cache Line。

- 指令是否为向量 Load。

不能把“4 Sectors/Request”当作所有访问的固定标准。

##### 15.5 Vectorized Load 的正确判断

将 4 次标量 Load 改成 `float4` 可能减少指令数，但需要：

- 地址满足 16-Byte 对齐。

- 每线程读取连续 16 Bytes。

- Warp 地址整体仍然连续。

- Tail 正确处理。

- 寄存器压力可接受。

应验证：

```text
SASS Load 指令数是否下降
Sector 数是否没有额外增加
LG Throttle 是否下降
Registers Per Thread 是否恶化
Duration 是否下降
```

#### 十六、Cache：Hit Rate 高为什么仍然可能很慢

Hit Rate 是最常被孤立解读的内存指标。

##### 16.1 Hit Rate 高不表示流量少

假设一个 Kernel 对同一批数据重复加载 100 次，全部命中 L2：

```text
L2 Hit Rate 很高
但 L2 -> L1 流量仍然巨大
Load 指令和 Scoreboard 依赖仍然存在
L2 或片上互连仍可能饱和
```

更好的优化是把数据提升到 Shared Memory 或寄存器中复用，而不是满足于 L2 Hit。

##### 16.2 Hit Rate 低不一定有问题

流式读取只使用一次的数据，本来就没有时间局部性：

```text
L1 Hit Rate 低
但访问完全合并
DRAM 带宽接近峰值
```

这可能已经是合理实现。强行缓存会污染 L1，并挤出真正有复用的数据。

##### 16.3 Hit Rate 要和 Bytes 一起看

设：

```text
L2 Requests = 1000
L2 Hits = 900
L2 Misses = 100
Hit Rate = 90%
```

另一个实现：

```text
L2 Requests = 100
L2 Hits = 50
L2 Misses = 50
Hit Rate = 50%
```

第二个实现 Hit Rate 更低，但总请求和 Miss 都更少，可能明显更快。

优先比较：

```text
绝对 Request/Sector/Byte
然后再看 Hit Rate
最后看 Duration
```

##### 16.4 L1 与 Shared Memory 的容量权衡

许多架构上 L1 和 Shared Memory 使用统一或可配置的片上容量。增加 Dynamic Shared Memory 可能改变可用 L1 容量和 Occupancy。

引入 Shared Tiling 后应该同时检查：

- DRAM Bytes 是否下降。

- L2/L1 Bytes 是否下降。

- Shared Bytes 和 Bank Conflict 是否上升。

- 每 CTA Shared Memory 是否降低 Occupancy。

- 总 Duration 是否下降。

#### 十七、Shared Memory：Bank Conflict 与 Wavefront

Shared Memory 通常由多个 Bank 组成。对常见的 32-Bank 组织，连续 FP32 Lane 访问通常映射到不同 Bank：

```text
bank = (byte_address / 4) % 32
```

具体 Bank 宽度和特殊访问规则应以目标架构文档为准。

##### 17.1 构造 32-Way Bank Conflict

```cpp
template <int Padding>
__global__ void shared_access(
    const float* __restrict__ input,
    float* __restrict__ output,
    int n) {
    extern __shared__ float raw_smem[];
    volatile float* smem = raw_smem;
    int global_index = blockIdx.x * blockDim.x + threadIdx.x;
    int warp = threadIdx.x / warpSize;
    int lane = threadIdx.x % warpSize;
    int stride = 32 + Padding;
    int warp_base = warp * warpSize * stride;
    int smem_index = warp_base + lane * stride;
    float value = global_index < n ? input[global_index] : 0.0f;
    smem[smem_index] = value;
    __syncwarp();
    if (global_index < n) {
        output[global_index] = smem[smem_index] + 1.0f;
    }
}
```

启动无 Padding 版本：

```cpp
constexpr int threads = 256;
int warps = threads / 32;
size_t bytes = warps * 32 * 32 * sizeof(float);
shared_access<0><<<blocks, threads, bytes>>>(input, output, n);
```

Lane 地址：

```text
lane 0  -> index 0
lane 1  -> index 32
lane 2  -> index 64
...
```

对 FP32 而言，这些地址都映射到同一个 Bank，但不是同一地址广播，因此会产生严重 Conflict。

加入一列 Padding：

```cpp
size_t bytes = warps * 32 * 33 * sizeof(float);
shared_access<1><<<blocks, threads, bytes>>>(input, output, n);
```

此时：

```text
bank = (lane * 33) % 32 = lane
```

不同 Lane 分散到不同 Bank。

##### 17.2 NCU 中看什么

常见指标方向：

```text
Shared Load Bank Conflicts
Shared Store Bank Conflicts
Shared Load/Store Transactions
Excessive Wavefronts
Short Scoreboard
MIO Throttle
L1/TEX Throughput
```

预期：

```text
Padding 后 Bank Conflict 和 Excessive Wavefront 下降
Short Scoreboard 或 MIO 压力下降
Duration 下降
```

##### 17.3 不要只看 Conflict 次数

Conflict 次数必须结合 Shared 指令总量。一个 Kernel 有少量 Conflict，但只执行一次；另一个 Kernel 每轮没有 Conflict，却执行了大量 Shared Load。后者仍可能更慢。

还要注意：

- 多个 Lane 读取同一地址可能走广播，不是 32-Way Conflict。

- 64-bit 或 128-bit 访问会跨多个 Bank。

- `ldmatrix`、Tensor Core Layout 和 Swizzle 有自己的访问模式。

- Padding 可能增加 Shared Memory 占用并降低 Occupancy。

#### 十八、Local Memory：识别 Register Spilling

CUDA Local Memory 是线程私有地址空间，但物理数据通常位于 Device Memory，并经过 L1/L2 Cache。它不是片上的“本地高速内存”。

##### 18.1 什么会进入 Local Memory

- 寄存器压力过大导致 Spill。

- 大型线程局部数组。

- 编译器无法静态展开索引的局部数组。

- 大型局部结构体。

- 函数调用和 Stack Frame。

编译时先看：

```bash
nvcc -O3 -lineinfo -Xptxas=-v kernel.cu -o kernel
```

典型输出字段：

```text
Used N registers
N bytes spill stores
N bytes spill loads
N bytes stack frame
```

##### 18.2 NCU 中看什么

常见 Base Metric 方向：

```text
l1tex__t_sectors_*_mem_local_op_ld
l1tex__t_sectors_*_mem_local_op_st
```

并结合：

```text
Local Load/Store Instructions
Long Scoreboard
LG Throttle
Registers Per Thread
Achieved Occupancy
Source/SASS 的 LDL、STL 或相关访问
```

##### 18.3 限制寄存器为什么可能更慢

使用：

```bash
nvcc --maxrregcount=64 ...
```

可能提高理论 Occupancy，但也可能把活跃值 Spill 到 Local Memory：

```text
Occupancy 上升
Local Sectors 上升
Long Scoreboard 上升
Duration 变长
```

因此优化目标不是最少寄存器，而是最短 Duration 下的寄存器、ILP 和 Occupancy 平衡。

##### 18.4 降低寄存器压力的常见方式

- 缩短变量 Live Range。

- 减少过度展开。

- 缩小每线程 Tile。

- 分阶段计算，避免同时保留所有中间值。

- 重新安排 Producer/Consumer Warp 的职责。

- 对只读值使用合理的共享或重新计算策略。

重新计算有时比 Spill 更便宜，但必须看增加了哪条 Compute Pipe 的压力。

#### 十九、Compute Workload：找到真正饱和的执行管线

Compute Workload Analysis 会显示 SM 上不同执行管线的利用率。常见类别包括：

```text
FP32 / FMA
FP64
Tensor
ALU / Integer
SFU / Special Function
Uniform
Load/Store 相关 Pipe
```

具体 Pipe 命名和能力随架构变化。

##### 19.1 高 Pipe Utilization 的含义

如果 FP32 Pipe 接近可持续上限，同时：

```text
Eligible Warps 充足
Scheduler 持续 Issued
Memory 没有更强限制
```

那么 FP32 指令吞吐很可能是瓶颈。

优化方向：

- 用 FMA 合并乘加。

- 减少重复算术。

- 使用更低精度。

- 将矩阵运算映射到 Tensor Core。

- 将某些整数或转换工作移出循环。

##### 19.2 Tensor Pipe 低不一定表示 Tensor Core 没用好

可能原因：

- Kernel 同时包含 Load、Softmax、Epilogue 等非 MMA 阶段。

- Tile 太小，MMA 在总 Duration 中占比低。

- Grid 太小，只有部分 SM 执行 MMA。

- Tensor Core 在 Active 周期很高，但 Elapsed 归一化较低。

- 数据准备或 Shared Memory 阻塞 Tensor Core。

应同时看：

```text
Tensor Pipe Active 与 Elapsed 归一化
MMA 指令数
Scheduler Eligible Warps
Short Scoreboard / MIO Throttle
Shared Memory Throughput
Source Timeline 或分阶段 Kernel
```

##### 19.3 SFU 或特殊数学成为瓶颈

`exp`、`log`、`sin`、倒数和某些转换可能使用特殊函数路径。Softmax、Normalization 或复杂激活中，FLOPs 数量不大，但特殊 Pipe 可能饱和。

优化方向：

- 使用硬件支持的近似指令。

- 用 `exp2` 重写，但正确处理缩放。

- 查表或多项式近似。

- 把特殊函数与独立 MMA/Load 重叠。

- 降低重复计算次数。

所有近似优化必须给出误差上界和端到端数值验证。

##### 19.4 Integer/ALU Pipe 高

常见于：

- 复杂 Tensor Index。

- 除法、取模。

- 64-bit 地址运算。

- 动态 Shape 和多级间接寻址。

- 稀疏格式解码。

优化方向：

- 预计算 Stride 和 Offset。

- 将除常数改为乘法、位移或模板常量。

- 使用增量 Pointer，而不是循环内重复构造多维索引。

- 将不变索引移到 CTA/Warp 共享层级。

- 在保证范围安全时减少不必要的 64-bit 算术。

##### 19.5 Pipe 低但 Execution Dependency 高

这表示吞吐上限没有打满，问题是指令依赖链，而不是指令数量。

常见改法：

- 多累加器。

- Loop Unroll 增加 ILP。

- 将 Load 提前若干 Iteration。

- 重排互不依赖的计算。

但 Unroll 会增加寄存器和代码尺寸，应重新检查：

```text
Registers
Spill
Occupancy
No Instruction
Duration
```

#### 二十、Instruction、Source 与 SASS：把指标定位到代码

高层 Section 告诉你“哪类资源有问题”，Source Page 才能回答“哪一行造成问题”。

##### 20.1 为什么必须看 SASS

CUDA 源码不是 GPU 最终执行的指令：

- 编译器可能消除、合并或移动代码。

- 短分支可能被 Predication。

- `float4` 不保证一定生成预期宽度的事务。

- 局部数组可能被放进寄存器，也可能进入 Local Memory。

- FMA 可能由乘加融合生成。

- 模板和常量传播会改变循环结构。

因此需要将 CUDA、PTX 和 SASS 对照。

##### 20.2 常见指令类别

不同架构助记符会变化，常见方向：

| 类别 | 常见含义 |
| --- | --- |
| `LDG` / Global Load | 从 Global 地址空间加载 |
| `STG` / Global Store | 写入 Global 地址空间 |
| `LDS` / Shared Load | 从 Shared Memory 加载 |
| `STS` / Shared Store | 写入 Shared Memory |
| `LDL` / Local Load | Local Memory 访问，可能是 Spill |
| `STL` / Local Store | Local Memory 写入，可能是 Spill |
| `FFMA` | FP32 Fused Multiply-Add |
| `HMMA` / `MMA` | Tensor Core 相关矩阵乘加 |
| `BRA` / `BRX` | 分支 |
| `BAR` | Barrier |

不要跨架构死记助记符，应在目标 GPU 的 Source/SASS 页面确认。

##### 20.3 Instruction Count 能说明什么

对同一输入和同一算法，Executed Instruction 明显下降通常是好信号。但还要区分：

- Warp-Level Instruction。

- Thread-Level Instruction。

- Predicated-On Thread Instruction。

- Issued 与 Executed。

例如一条 Warp 指令只让 8 个 Lane 有效：

```text
Warp Instruction Count 看起来正常
Predicated-On Thread 数量很低
实际线程利用率差
```

##### 20.4 Source Counters 的正确用法

在 Source Page 叠加：

```text
Warp Stall Samples
Global/Local/Shared Memory 指标
Branch Targets
Predicated-On Threads
Instruction Count
```

然后沿最重的源码行检查：

```text
这行生成了哪些 SASS？
它的输入来自哪个 Load？
它是否位于循环内部？
每个线程都会执行吗？
是否被边界 Predicate 屏蔽？
```

Stall 经常显示在“等待结果的消费指令”上，而不是最初发出 Load 的源码行。分析时要向前追数据依赖。

#### 二十一、Branch 与线程利用率：识别 Warp Divergence

Warp Divergence 发生在同一个 Warp 的 Lane 走不同控制流路径。硬件需要分别执行各路径，并通过 Active Mask 屏蔽不属于当前路径的 Lane。

##### 21.1 一个简单对照

发散版本：

```cpp
__global__ void divergent(
    const float* input,
    float* output,
    int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) {
        return;
    }
    float x = input[i];
    int lane = threadIdx.x & 31;
    if (lane < 16) {
        #pragma unroll 1
        for (int k = 0; k < 32; ++k) {
            x = fmaf(x, 1.0001f, 0.0001f);
        }
    } else {
        #pragma unroll 1
        for (int k = 0; k < 32; ++k) {
            x = __fdividef(x + 0.0001f, 1.0001f);
        }
    }
    output[i] = x;
}
```

同一个 Warp 中一半 Lane 走 FMA，另一半走 Divide，两条路径都需要执行。

如果把条件改为 Warp 粒度：

```cpp
int warp_global = i / 32;
if ((warp_global & 1) == 0) {
    // 整个 Warp 走 FMA
} else {
    // 整个 Warp 走 Divide
}
```

总工作分布近似不变，但 Warp 内不再分裂。

##### 21.2 NCU 中看什么

- Branch Efficiency 或相关 Source Counter。

- Avg. Active Threads Per Warp。

- Avg. Not Predicated-Off Threads Per Warp。

- Branch Instructions。

- Source 上每条路径的 Active Mask。

- Warp Stall 中 Branch Resolving。

- Duration。

##### 21.3 短分支可能不会生成真正分支

编译器可能使用 Predication：

```cpp
if (condition) {
    y = x + 1.0f;
}
```

可能变成所有 Lane 发出同一条指令，仅通过 Predicate 决定哪些 Lane 写回。此时 Branch Instruction 不一定增加，但 Predicated-On Thread 利用率会下降。

所以不能只看 Branch Count。

##### 21.4 边界分支通常不是主要问题

```cpp
if (i < n) {
    ...
}
```

通常只有最后一个 Warp 或少量边界 CTA 发散，占比很小。除非 Tensor 很小、边界 Tile 很多或 Padding 浪费显著，否则不应优先优化它。

更值得关注的是循环内部、每个 Tile 都执行的数据相关分支。

#### 二十二、四类典型报告如何推导优化动作

下面不使用固定数值，而是强调指标之间的因果关系。

##### 22.1 案例一：连续 Vector Add

报告特征：

```text
Grid 足够大
DRAM Throughput 高
Global Sectors/Request 接近理想
Compute Pipe 低
Eligible Warp 尚可
```

结论：

```text
访问已经合并，Kernel 接近 DRAM Bandwidth-bound
```

低价值改动：

- 继续增加 Block 数。

- 把简单 Add 手工展开很多次。

- 只追求更高 Occupancy。

高价值方向：

- 与前后算子融合。

- 减少输入或输出。

- 使用更窄的数据类型。

- 确认是否可以 In-place。

##### 22.2 案例二：跨步 Gather

报告特征：

```text
Global Load Sectors/Request 高
L1/TEX Wavefront 多
Long Scoreboard 高
DRAM Throughput 未必达到峰值
Eligible Warp 少
```

结论：

```text
每条 Load 的空间利用率差，且 Warp 在等待高延迟、不规则访问
这是访问效率和延迟问题，不一定是带宽饱和
```

优化方向：

- 交换线程与数据维度映射。

- 预转置或改变 Layout。

- 让 Warp 访问连续维。

- Shared Memory 中做局部重排。

- 对索引排序或分桶，提高局部性。

##### 22.3 案例三：高寄存器 GEMM

报告特征：

```text
Registers Per Thread 高
Theoretical Occupancy 低
但 Tensor Pipe 高
Eligible Warp 足够
Local Load/Store 为 0
```

结论：

```text
低 Occupancy 可能是高寄存器复用的合理代价
当前 Tensor Core 已被充分供给
```

危险改动：

```text
强制 maxrregcount
-> Occupancy 上升
-> Spill 出现
-> Long Scoreboard 上升
-> Duration 变差
```

应先优化：

- MMA 与 Load Pipeline 重叠。

- Shared Memory Layout。

- Epilogue。

- Tile 与 Wave Quantization。

##### 22.4 案例四：归约 Kernel Barrier 很高

报告特征：

```text
Compute 与 DRAM 都不高
Active Warp 不少
Eligible Warp 少
Barrier Stall 高
不同 Warp 的工作量不一致
```

结论：

```text
CTA 内同步和负载不均阻止 Scheduler 获得 Ready Warp
```

优化方向：

- Warp Shuffle 替代部分 Shared Reduction。

- 先做 Warp 内归约，只让每 Warp 一个 Lane 写 Shared。

- 减少 `__syncthreads()` 次数。

- 避免交错寻址分支。

- 对尾部使用 Warp 同步。

验证：

```text
Barrier Stall 下降
Shared 指令下降
Eligible Warps 上升
Duration 下降
```

#### 二十三、最容易误读的 NCU 指标

##### 23.1 “Occupancy 越高越好”

错误。Occupancy 是延迟隐藏的容量，不是实际吞吐。高性能 GEMM 常用大量寄存器保存 Accumulator，Occupancy 不高但 Tensor Core 很忙。

##### 23.2 “Long Scoreboard 高就是 DRAM Bandwidth-bound”

错误。它表示等待内存相关依赖。Pointer Chasing 可能 Long Scoreboard 很高，但 DRAM 带宽很低，因为并发请求不足。

##### 23.3 “Memory Throughput 高就是 HBM 满了”

错误。高值可能来自 L1/TEX、L2、Shared 或某条片上数据路径。必须展开 Memory Chart。

##### 23.4 “L2 Hit Rate 越高越好”

错误。Hit Rate 不反映总请求数。请求减少一半但 Hit Rate 下降，仍可能更快。

##### 23.5 “Not Selected 是 Warp 被浪费”

通常错误。它经常说明有多个 Eligible Warp，Scheduler 正常选择其中一个发射。

##### 23.6 “某个 Stall 占 40%，消掉后就能快 40%”

错误。Warp Stall 的分母通常不是 Kernel 墙钟时间，且多个 Warp 状态重叠。消除一个 Stall 后，其他瓶颈会接管。

##### 23.7 “Compute Throughput 低说明计算优化空间大”

错误。Memory-bound Kernel 本来就不需要高 Compute Throughput。优化计算不会降低 DRAM 时间。

##### 23.8 “用了 Shared Memory 就减少了 DRAM”

不一定。若原数据已命中 L1/L2，或 Shared Tile 没有被复用，可能只是增加了一次搬运、Barrier 和 Shared 流量。

##### 23.9 “Vectorized Load 一定减少内存流量”

不一定。它主要减少指令数和请求组织开销。若地址不对齐、Tail 处理不当或线程映射不连续，Sector 数可能不降反升。

##### 23.10 “`--set full` 的报告最可信”

错误。`full` 只是指标更多，也带来更多 Replay、保存恢复和运行扰动。针对假设采集最少必要 Section，通常更容易得到稳定结论。

#### 二十四、一套可复用的算子优化流程

##### 24.1 优化前先写理论账本

对给定 Shape 计算：

```text
理论 FLOPs
最少 DRAM Read/Write Bytes
输出 Tile 数
CTA 数与 Waves
每个元素理想访问次数
允许的数值误差
```

没有理论账本，就无法判断 NCU 中的 Bytes 和 Instructions 是必要工作还是浪费。

##### 24.2 第一轮：分类

采集：

```text
SpeedOfLight
LaunchStats
Occupancy
```

回答：

```text
Grid 够不够？
哪种资源限制驻留？
Compute 或 Memory 是否有明显高值？
```

##### 24.3 第二轮：确认调度状态

采集：

```text
SchedulerStats
WarpStateStats
```

回答：

```text
Active Warp 是否足够？
Eligible Warp 是否足够？
Issue Slot 是否经常空闲？
主要 Stall 是依赖、同步还是 Throttle？
```

##### 24.4 第三轮：深挖限制子系统

Memory 路径：

```text
MemoryWorkloadAnalysis
SourceCounters
```

检查：

```text
Request
Sector/Request
Wavefront
Cache Bytes/Hit
DRAM Bytes/Throughput
Shared Conflict
Local Memory
```

Compute 路径：

```text
ComputeWorkloadAnalysis
InstructionStats
SourceCounters
```

检查：

```text
具体 Pipe
SASS Mix
Predicated-On Threads
Dependency
转换与索引指令
```

##### 24.5 为每次修改定义可证伪预测

示例：

| 修改 | 预期 NCU 变化 | 最终目标 |
| --- | --- | --- |
| Warp 连续访问 | Sectors/Request 下降 | Duration 下降 |
| Shared Padding | Bank Conflict 下降 | Duration 下降 |
| 多累加器 | Execution Dependency 下降 | Pipe 利用率提高 |
| 减少 Unroll | Registers/Spill 下降 | Duration 下降 |
| Split-K | Waves 增加 | 总 GEMM + Reduction 下降 |
| Kernel Fusion | DRAM Bytes 下降 | 端到端延迟下降 |

如果预测没有发生，说明对编译结果或硬件行为的假设不成立，应回到 SASS 和原始 Metric。

##### 24.6 用 Baseline 比绝对阈值更可靠

不同 GPU、频率、Shape 和 NCU 版本之间，绝对百分比不总可比。最可靠的比较是：

```text
同一机器
同一 Shape
同一输入
同一编译选项
同一 NCU Section 和 Replay 配置
Baseline vs Candidate
```

重点比较三层：

```text
工作量:
  Bytes、Sectors、Instructions、Transactions
效率:
  Throughput、Eligible Warp、Stall、Pipe Utilization
结果:
  无 Profiler Duration、吞吐、端到端延迟、数值误差
```

##### 24.7 最后的检查清单

```text
[ ] Kernel 确实是端到端热点
[ ] Release 编译并带 lineinfo
[ ] 输入、Shape、时钟和缓存语义可复现
[ ] Grid/Waves 足够，或已识别并行度问题
[ ] SOL 只用于分类，没有直接下结论
[ ] Roofline 的 FLOPs 和 Bytes 层级明确
[ ] Occupancy 与 Eligible Warp 联合分析
[ ] Stall 与 Scheduler Issue Slot 联合分析
[ ] Memory 从 Request/Sector 追到 DRAM
[ ] Cache Hit Rate 与绝对 Bytes 联合分析
[ ] Source 指标已经追到 SASS 和数据依赖
[ ] 每个修改都有预期指标变化
[ ] 最终性能在无 NCU 环境下复测
[ ] 数值正确性和误差没有退化
```

#### 二十五、参考资料

1. [NVIDIA Nsight Compute Profiling Guide](https://docs.nvidia.com/nsight-compute/ProfilingGuide/)

1. [NVIDIA Nsight Compute CLI User Guide](https://docs.nvidia.com/nsight-compute/NsightComputeCli/)

1. [NVIDIA Nsight Compute User Guide](https://docs.nvidia.com/nsight-compute/NsightCompute/)

1. [NVIDIA Nsight Compute Release Notes](https://docs.nvidia.com/nsight-compute/ReleaseNotes/)

1. [NVIDIA CUDA C++ Best Practices Guide](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)

1. [NVIDIA CUDA Programming Guide](https://docs.nvidia.com/cuda/cuda-programming-guide/)

1. [NVIDIA Nsight Compute Roofline Analysis](https://developer.nvidia.com/blog/accelerating-hpc-applications-with-nsight-compute-roofline-analysis/)

1. [Nsight Compute Kernel Profiling Guide: Memory Chart and Tables](https://docs.nvidia.com/nsight-compute/ProfilingGuide/#memory-chart)

NCU 的价值不在于把所有百分比都调高，而在于建立一条可验证的因果链：

```text
算法需要多少工作
-> 实际产生了多少指令和内存事务
-> 哪个硬件单元最接近上限
-> Scheduler 为什么无法继续发射
-> 哪一行源码和哪条 SASS 造成等待
-> 修改后工作量、效率和最终延迟是否同时改善
```

成熟的 CUDA 优化不是看到 `Occupancy` 低就提高 Occupancy，也不是看到 `Long Scoreboard` 高就堆更多 Warp。真正有效的方法是先判断 Throughput、Latency 和 Parallelism 中谁在限制性能，再用 NCU 的多组指标互相验证，最后回到无 Profiler 的端到端结果。

## 5 手写 CUDA 算子

### 手写 CUDA Vector Add 与 Vector Dot Product

> 原文：[手写 CUDA Vector Add 与 Vector Dot Product](https://hd9568.github.io/blog/cuda-vector-add-dot-product/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**从最基础的向量加法开始，逐步写出 Dot Product 的并行规约版本，理解线程映射、边界检查和块级归约。

#### 一、核心结论

Vector Add 和 Dot Product 是 CUDA 手写算子的入门题。

- Vector Add 是一线程处理一个元素，重点是 global id 和边界检查。

- Dot Product 需要先逐元素相乘，再并行求和。

- 求和不能让所有线程直接 `atomicAdd` 到同一个地址，否则竞争很重。

- 常见做法是每个 Block 先规约出一个 partial sum，再对 partial sum 做二次规约。

- 对大数组，访存合并比计算本身更重要。

#### 二、Vector Add

Kernel：

```cpp
__global__ void vector_add(const float* a,
                           const float* b,
                           float* c,
                           int n) {
    // 全局线程编号：每个线程负责一个元素。
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // Grid 通常向上取整，线程数可能超过 n，因此必须检查边界。
    if (idx < n) {
        // 相邻线程访问相邻元素，Global Memory 访问容易合并。
        c[idx] = a[idx] + b[idx];
    }
}
```

这个 kernel 的特点：

- 每个元素独立，没有线程间同步。

- 算术强度低，通常是 Memory-bound。

- 优化重点是合并访问和减少额外读写。

#### 三、Host 端启动方式

```cpp
int threads = 256;
int blocks = (n + threads - 1) / threads;
vector_add<<<blocks, threads>>>(d_a, d_b, d_c, n);
cudaDeviceSynchronize();
```

为什么要向上取整：

```cpp
(n + threads - 1) / threads
```

如果 `n = 1000`，`threads = 256`，需要 4 个 Block，总线程数 1024，多出的 24 个线程通过 `if (idx < n)` 被屏蔽。

#### 四、Vector Dot Product 的拆分思路

Dot Product：

```text
sum = a[0] * b[0] + a[1] * b[1] + ... + a[n-1] * b[n-1]
```

并行化思路：

1. 每个线程计算若干个 `a[i] * b[i]`。

1. Block 内把这些乘积规约成一个 partial sum。

1. 每个 Block 写出一个 partial sum。

1. 再对 partial sum 求和。

#### 五、块内规约版本

```cpp
__global__ void dot_product_partial(const float* a,
                                    const float* b,
                                    float* partial,
                                    int n) {
    extern __shared__ float smem[];
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    float val = 0.0f;
    if (idx < n) {
        // 每个线程先计算一个乘积。
        val = a[idx] * b[idx];
    }
    // 把每个线程的局部结果放入 shared memory。
    smem[tid] = val;
    __syncthreads();
    // 块内并行规约。stride 每次减半。
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (tid < stride) {
            smem[tid] += smem[tid + stride];
        }
        __syncthreads();
    }
    // 每个 block 输出一个 partial sum。
    if (tid == 0) {
        partial[blockIdx.x] = smem[0];
    }
}
```

启动：

```cpp
int threads = 256;
int blocks = (n + threads - 1) / threads;
size_t shared_bytes = threads * sizeof(float);
dot_product_partial<<<blocks, threads, shared_bytes>>>(d_a, d_b, d_partial, n);
```

`d_partial` 长度是 `blocks`。后续可以：

- 再启动一个 reduction kernel 规约 `d_partial`。

- 或者把 `partial` 拷回 Host 求和（只适合 blocks 很少的场景）。

#### 六、优化要点

##### 1. Grid-stride loop

如果 `n` 很大，或者希望每个线程处理多个元素：

```cpp
for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
    val += a[i] * b[i];
}
```

这样可以减少 Block 数限制带来的问题，也提高每线程工作量。

##### 2. Warp shuffle 优化尾部规约

Shared Memory 规约到 32 个线程后，可以用 `__shfl_down_sync` 做 Warp 内规约，减少 `__syncthreads()`。

##### 3. 避免全局 atomicAdd 热点

```cpp
// 不推荐：所有线程竞争同一个地址。
atomicAdd(result, a[idx] * b[idx]);
```

大量线程对同一地址原子加，会严重串行化。更好的方式是 Block 先局部规约。

#### 七、面试回答模板

如果问题是“怎么手写 Vector Add / Dot Product”，可以这样回答：

1. Vector Add 一线程处理一个元素，global id 是 `blockIdx.x * blockDim.x + threadIdx.x`。

1. 由于 Grid 向上取整，kernel 内要做 `idx < n` 边界检查。

1. Dot Product 先每线程计算乘积，再做并行规约。

1. 常见做法是每个 Block 输出 partial sum，再对 partial sum 二次规约。

1. 不建议所有线程直接 `atomicAdd` 到同一个全局结果，因为竞争开销很大。

#### 八、总结

Vector Add 检查线程映射和访存合并，Dot Product 检查并行规约能力。这两个题虽然简单，但覆盖了 CUDA 手写 kernel 的基本功：线程编号、边界检查、shared memory、同步和规约。

### 手写 CUDA Parallel Reduction：从交错寻址到 Warp 级规约

> 原文：[手写 CUDA Parallel Reduction：从交错寻址到 Warp 级规约](https://hd9568.github.io/blog/cuda-parallel-reduction/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**系统拆解 CUDA 并行规约，从朴素交错寻址、连续寻址、Shared Memory 到 Warp Shuffle 和循环展开。

#### 一、核心结论

Parallel Reduction 是 CUDA 高频手撕题，核心是把很多元素并行求和。

- 朴素版本容易出现线程利用率低、分支和 Bank Conflict。

- 连续寻址规约比交错寻址更友好。

- 每个线程加载多个元素可以减少 Block 数和同步次数。

- Warp 内规约可以用 `__shfl_down_sync`，避免 shared memory 和 `__syncthreads()`。

- 高性能版本通常结合连续寻址、展开、Warp shuffle 和多阶段规约。

#### 二、Reduction 要解决什么

目标：

```text
out = x[0] + x[1] + ... + x[n-1]
```

并行思路：

```text
全局数组 -> 每个 Block 求 partial sum -> partial sums 再规约
```

#### 三、交错寻址版本

这是容易理解但不够高效的版本。

```cpp
__global__ void reduce_interleaved(const float* input, float* partial, int n) {
    extern __shared__ float smem[];
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + tid;
    smem[tid] = (idx < n) ? input[idx] : 0.0f;
    __syncthreads();
    // stride 从 1 开始，每轮活跃线程变少。
    for (int stride = 1; stride < blockDim.x; stride *= 2) {
        if ((tid % (2 * stride)) == 0) {
            smem[tid] += smem[tid + stride];
        }
        __syncthreads();
    }
    if (tid == 0) {
        partial[blockIdx.x] = smem[0];
    }
}
```

问题：

- `%` 取模开销较高。

- 活跃线程分布不连续，容易造成分支和访存不友好。

- 后期大量线程闲置。

#### 四、连续寻址版本

更常见写法是 stride 从一半开始减半。

```cpp
__global__ void reduce_contiguous(const float* input, float* partial, int n) {
    extern __shared__ float smem[];
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + tid;
    smem[tid] = (idx < n) ? input[idx] : 0.0f;
    __syncthreads();
    // 活跃线程连续：tid < stride。
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (tid < stride) {
            smem[tid] += smem[tid + stride];
        }
        __syncthreads();
    }
    if (tid == 0) {
        partial[blockIdx.x] = smem[0];
    }
}
```

优点：

- 活跃线程连续。

- 分支更简单。

- Shared Memory 访问更规整。

#### 五、一次加载两个元素

每个线程先加两个元素，可以减少一半 Block 数。

```cpp
__global__ void reduce_two_loads(const float* input, float* partial, int n) {
    extern __shared__ float smem[];
    int tid = threadIdx.x;
    int idx = blockIdx.x * (blockDim.x * 2) + tid;
    float sum = 0.0f;
    if (idx < n) {
        sum += input[idx];
    }
    if (idx + blockDim.x < n) {
        sum += input[idx + blockDim.x];
    }
    smem[tid] = sum;
    __syncthreads();
    for (int stride = blockDim.x / 2; stride > 32; stride >>= 1) {
        if (tid < stride) {
            smem[tid] += smem[tid + stride];
        }
        __syncthreads();
    }
    // 最后一个 warp 用 shuffle 规约。
    float val = smem[tid];
    if (tid < 32) {
        unsigned mask = 0xffffffff;
        val += __shfl_down_sync(mask, val, 16);
        val += __shfl_down_sync(mask, val, 8);
        val += __shfl_down_sync(mask, val, 4);
        val += __shfl_down_sync(mask, val, 2);
        val += __shfl_down_sync(mask, val, 1);
    }
    if (tid == 0) {
        partial[blockIdx.x] = val;
    }
}
```

#### 六、Warp 级规约

独立函数：

```cpp
__inline__ __device__ float warp_reduce_sum(float val) {
    unsigned mask = 0xffffffff;
    val += __shfl_down_sync(mask, val, 16);
    val += __shfl_down_sync(mask, val, 8);
    val += __shfl_down_sync(mask, val, 4);
    val += __shfl_down_sync(mask, val, 2);
    val += __shfl_down_sync(mask, val, 1);
    return val;
}
```

它的特点：

- Warp 内线程交换寄存器值。

- 不需要 shared memory。

- 不需要 `__syncthreads()`，因为 Warp 内同步执行。

#### 七、完全展开思路

如果 `blockDim.x` 固定为 256，可以手写展开：

```cpp
if (blockDim.x >= 256) {
    if (tid < 128) smem[tid] += smem[tid + 128];
    __syncthreads();
}
if (blockDim.x >= 128) {
    if (tid < 64) smem[tid] += smem[tid + 64];
    __syncthreads();
}
```

完全展开减少循环控制开销，但会增加代码体积。实际高性能库通常用模板参数控制 Block 大小，在编译期生成展开版本。

#### 八、面试回答模板

如果问题是“怎么优化 Parallel Reduction”，可以这样回答：

1. 先让每个 Block 对一段数据做局部规约，输出 partial sum。

1. 交错寻址版本简单但有取模、分支和线程利用率问题。

1. 连续寻址版本让活跃线程连续，访存和分支更友好。

1. 每个线程加载两个或多个元素，减少 Block 数和同步次数。

1. 最后一个 Warp 用 `__shfl_down_sync` 做规约，避免 shared memory 和同步。

1. 固定 Block 大小时可以循环展开，但要注意寄存器和代码体积。

#### 九、总结

Reduction 的优化路线很清晰：

```text
朴素 shared memory -> 连续寻址 -> 多元素加载 -> Warp shuffle -> 展开
```

它考察的不只是代码，而是对线程利用率、同步成本、shared memory 和 Warp 原语的综合理解。

### 手写 CUDA Softmax：Online Softmax 与数值稳定性

> 原文：[手写 CUDA Softmax：Online Softmax 与数值稳定性](https://hd9568.github.io/blog/cuda-online-softmax/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**从数值稳定 Softmax 写法出发，理解 Online Softmax 如何减少访存，并给出单行 Softmax kernel 的清晰实现思路。

#### 一、核心结论

Softmax 是 Attention 中非常核心的算子。

- 直接计算 `exp(x)` 可能溢出，需要先减去最大值。

- 稳定 Softmax 通常需要求 max、求 sum、再归一化。

- 朴素实现会多次读写 Global Memory。

- Online Softmax 可以边扫描边更新 max 和 sum，减少访存或适配分块场景。

- 高性能 Softmax 通常结合 Warp/Block 规约、向量化访问和寄存器缓存。

#### 二、Softmax 的数值稳定写法

定义：

```text
softmax(x_i) = exp(x_i) / sum_j exp(x_j)
```

如果 `x_i` 很大，`exp(x_i)` 可能溢出。稳定写法：

```text
m = max(x)
softmax(x_i) = exp(x_i - m) / sum_j exp(x_j - m)
```

减去最大值不改变结果，因为分子分母同时除以 `exp(m)`。

#### 三、朴素三阶段 Softmax

对一行长度为 `cols` 的数据：

1. 求最大值 `max_val`。

1. 求 `sum(exp(x_i - max_val))`。

1. 写出 `exp(x_i - max_val) / sum`。

朴素伪代码：

```cpp
for each row:
    max_val = max(x[row])
    denom = sum(exp(x[row][i] - max_val))
    y[row][i] = exp(x[row][i] - max_val) / denom
```

问题是同一行数据至少被读两遍，输出再写一遍。对于长序列，访存成本很明显。

#### 四、Online Softmax 的思想

Online Softmax 在扫描元素时同时维护当前最大值 `m` 和归一化分母 `d`。

假设已有前缀的：

```text
m_old = max(previous values)
d_old = sum(exp(x_i - m_old))
```

新来一个值 `x`：

```text
m_new = max(m_old, x)
d_new = d_old * exp(m_old - m_new) + exp(x - m_new)
```

为什么要乘 `exp(m_old - m_new)`：

- 旧的 `d_old` 是基于 `m_old` 归一化的。

- 如果最大值变成 `m_new`，旧分母要换到新的基准。

这让 max 和 sum 可以在一次扫描中更新。

#### 五、单 Block 处理一行的示例

下面示例适合一行长度不太大、一个 Block 处理一行的教学版本。

```cpp
__inline__ __device__ float warp_reduce_max(float v) {
    unsigned mask = 0xffffffff;
    v = fmaxf(v, __shfl_down_sync(mask, v, 16));
    v = fmaxf(v, __shfl_down_sync(mask, v, 8));
    v = fmaxf(v, __shfl_down_sync(mask, v, 4));
    v = fmaxf(v, __shfl_down_sync(mask, v, 2));
    v = fmaxf(v, __shfl_down_sync(mask, v, 1));
    return v;
}
__inline__ __device__ float warp_reduce_sum(float v) {
    unsigned mask = 0xffffffff;
    v += __shfl_down_sync(mask, v, 16);
    v += __shfl_down_sync(mask, v, 8);
    v += __shfl_down_sync(mask, v, 4);
    v += __shfl_down_sync(mask, v, 2);
    v += __shfl_down_sync(mask, v, 1);
    return v;
}
__global__ void softmax_one_block_per_row(const float* x, float* y, int rows, int cols) {
    extern __shared__ float smem[];
    int row = blockIdx.x;
    int tid = threadIdx.x;
    if (row >= rows) return;
    const float* row_x = x + row * cols;
    float* row_y = y + row * cols;
    // 每个线程处理多个列元素，先求自己的局部最大值。
    float local_max = -INFINITY;
    for (int col = tid; col < cols; col += blockDim.x) {
        local_max = fmaxf(local_max, row_x[col]);
    }
    // 先做 warp 内 max。
    float warp_max = warp_reduce_max(local_max);
    // 每个 warp 的 lane 0 把结果写到 shared memory。
    int lane = tid % 32;
    int warp_id = tid / 32;
    if (lane == 0) {
        smem[warp_id] = warp_max;
    }
    __syncthreads();
    // 第一个 warp 规约所有 warp 的结果。
    float block_max = -INFINITY;
    if (warp_id == 0) {
        block_max = (tid < (blockDim.x + 31) / 32) ? smem[lane] : -INFINITY;
        block_max = warp_reduce_max(block_max);
    }
    // 广播 block_max。
    if (tid == 0) smem[0] = block_max;
    __syncthreads();
    block_max = smem[0];
    // 求 exp 和 sum。这里把 exp 结果先写到 y，避免第三次读 x。
    float local_sum = 0.0f;
    for (int col = tid; col < cols; col += blockDim.x) {
        float v = expf(row_x[col] - block_max);
        row_y[col] = v;
        local_sum += v;
    }
    float warp_sum = warp_reduce_sum(local_sum);
    if (lane == 0) {
        smem[warp_id] = warp_sum;
    }
    __syncthreads();
    float block_sum = 0.0f;
    if (warp_id == 0) {
        block_sum = (tid < (blockDim.x + 31) / 32) ? smem[lane] : 0.0f;
        block_sum = warp_reduce_sum(block_sum);
    }
    if (tid == 0) smem[0] = block_sum;
    __syncthreads();
    block_sum = smem[0];
    // 归一化输出。
    for (int col = tid; col < cols; col += blockDim.x) {
        row_y[col] = row_y[col] / block_sum;
    }
}
```

这个版本便于理解，但不是最终极性能版本。长序列 Attention 中通常还要结合分块、Online Softmax 和 FlashAttention 思路。

#### 六、性能优化要点

- 对每行做 max 和 sum 规约。

- 使用 Warp shuffle 减少 shared memory 和同步开销。

- 对较短行，一个 Block 处理一行较简单。

- 对很长行，需要多 Block 协作或分块 Online Softmax。

- 减少 Global Memory 往返，尽量把中间值留在寄存器或 shared memory。

- 使用近似 `exp` 或向量化时要注意精度要求。

#### 七、面试回答模板

如果问题是“Softmax 为什么要减 max”，可以回答：

1. 直接 `exp(x)` 可能溢出，减去最大值可以保证指数输入不大于 0。

1. Softmax 对整体平移不敏感，减 max 不改变最终结果。

1. 朴素稳定 Softmax 需要求 max、求 sum、归一化。

1. Online Softmax 能在扫描时同时更新 max 和 sum，适合分块处理，减少访存。

1. CUDA 实现中通常用 Warp/Block 规约求 max 和 sum。

#### 八、总结

Softmax 的关键不是公式，而是数值稳定和访存控制。面试中能讲清“减 max 防溢出”“max/sum 两类规约”“Online 更新公式”“CUDA 中如何做 Warp/Block 规约”，就已经覆盖核心逻辑。

### 手写 CUDA LayerNorm / RMSNorm：均值与方差的高效计算

> 原文：[手写 CUDA LayerNorm / RMSNorm：均值与方差的高效计算](https://hd9568.github.io/blog/cuda-layernorm-rmsnorm/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**从归一化公式出发，理解 LayerNorm 和 RMSNorm 的区别，并用单 Block 单行的 CUDA kernel 讲清均值、方差与规约实现。

#### 一、核心结论

LayerNorm 和 RMSNorm 是 LLM 中非常高频的归一化算子。

- LayerNorm 对一行 hidden 维度求均值和方差。

- RMSNorm 不减均值，只用平方均值计算 RMS。

- 二者都需要对一行数据做规约。

- CUDA 实现常用一个 Block 处理一行，线程并行求 sum / sum of squares。

- 优化重点是规约效率、访存次数、向量化加载和融合后处理。

#### 二、LayerNorm 与 RMSNorm 公式

LayerNorm：

```text
mean = sum(x_i) / H
var = sum((x_i - mean)^2) / H
y_i = (x_i - mean) / sqrt(var + eps) * gamma_i + beta_i
```

RMSNorm：

```text
rms = sqrt(sum(x_i^2) / H + eps)
y_i = x_i / rms * weight_i
```

区别：

- LayerNorm 需要均值和方差。

- RMSNorm 不计算均值，少一次中心化，通常更简单更快。

- RMSNorm 在很多 LLM 中常见。

#### 三、计算流程

以一行 hidden size 为 `H` 的数据为例。

LayerNorm：

1. 多线程读入 `x`，求 `sum(x)`。

1. 得到 `mean`。

1. 多线程求 `sum((x - mean)^2)`。

1. 得到 `inv_std = rsqrt(var + eps)`。

1. 写出归一化结果。

RMSNorm：

1. 多线程求 `sum(x^2)`。

1. 得到 `inv_rms = rsqrt(sum_sq / H + eps)`。

1. 写出 `x * inv_rms * weight`。

#### 四、LayerNorm CUDA 示例

下面是单 Block 处理一行的教学版本。

```cpp
__inline__ __device__ float warp_reduce_sum(float v) {
    unsigned mask = 0xffffffff;
    v += __shfl_down_sync(mask, v, 16);
    v += __shfl_down_sync(mask, v, 8);
    v += __shfl_down_sync(mask, v, 4);
    v += __shfl_down_sync(mask, v, 2);
    v += __shfl_down_sync(mask, v, 1);
    return v;
}
__device__ float block_reduce_sum(float v, float* smem) {
    int tid = threadIdx.x;
    int lane = tid % 32;
    int warp_id = tid / 32;
    v = warp_reduce_sum(v);
    if (lane == 0) {
        smem[warp_id] = v;
    }
    __syncthreads();
    float out = 0.0f;
    if (warp_id == 0) {
        out = (tid < (blockDim.x + 31) / 32) ? smem[lane] : 0.0f;
        out = warp_reduce_sum(out);
    }
    if (tid == 0) {
        smem[0] = out;
    }
    __syncthreads();
    return smem[0];
}
__global__ void layernorm_kernel(const float* x,
                                 const float* gamma,
                                 const float* beta,
                                 float* y,
                                 int rows,
                                 int hidden,
                                 float eps) {
    extern __shared__ float smem[];
    int row = blockIdx.x;
    int tid = threadIdx.x;
    if (row >= rows) return;
    const float* row_x = x + row * hidden;
    float* row_y = y + row * hidden;
    // 第一次规约：求 sum。
    float local_sum = 0.0f;
    for (int i = tid; i < hidden; i += blockDim.x) {
        local_sum += row_x[i];
    }
    float sum = block_reduce_sum(local_sum, smem);
    float mean = sum / hidden;
    // 第二次规约：求方差。
    float local_var = 0.0f;
    for (int i = tid; i < hidden; i += blockDim.x) {
        float diff = row_x[i] - mean;
        local_var += diff * diff;
    }
    float var_sum = block_reduce_sum(local_var, smem);
    float inv_std = rsqrtf(var_sum / hidden + eps);
    // 写出归一化结果。
    for (int i = tid; i < hidden; i += blockDim.x) {
        float norm = (row_x[i] - mean) * inv_std;
        row_y[i] = norm * gamma[i] + beta[i];
    }
}
```

#### 五、RMSNorm CUDA 示例

RMSNorm 少了求均值和 beta。

```cpp
__global__ void rmsnorm_kernel(const float* x,
                               const float* weight,
                               float* y,
                               int rows,
                               int hidden,
                               float eps) {
    extern __shared__ float smem[];
    int row = blockIdx.x;
    int tid = threadIdx.x;
    if (row >= rows) return;
    const float* row_x = x + row * hidden;
    float* row_y = y + row * hidden;
    // 求平方和。
    float local_sq_sum = 0.0f;
    for (int i = tid; i < hidden; i += blockDim.x) {
        float v = row_x[i];
        local_sq_sum += v * v;
    }
    float sq_sum = block_reduce_sum(local_sq_sum, smem);
    float inv_rms = rsqrtf(sq_sum / hidden + eps);
    // 写出结果。
    for (int i = tid; i < hidden; i += blockDim.x) {
        row_y[i] = row_x[i] * inv_rms * weight[i];
    }
}
```

RMSNorm 比 LayerNorm 少一次中心化和一次减法相关计算，因此实现更简单。

#### 六、性能优化要点

##### 1. 减少访存次数

LayerNorm 需要至少多次读取同一行。若 hidden 较小，可以把数据缓存到 shared memory 或寄存器中，减少重复 Global Memory 访问。

##### 2. 向量化加载

对 hidden 维度较大且地址对齐时，可以用 `float4` 加载，提高带宽利用率。

##### 3. Warp 级规约

对 hidden 较小的场景，Warp 级规约可能比 Block 级更轻。

##### 4. 融合后处理

LayerNorm/RMSNorm 常和 residual、bias、activation 融合，减少中间结果写回。

##### 5. 精度处理

输入可能是 FP16/BF16，但规约通常用 FP32 累加，减少数值误差。

#### 七、面试回答模板

如果问题是“LayerNorm 和 RMSNorm 怎么实现”，可以这样回答：

1. LayerNorm 对每一行 hidden 维度求均值和方差，再做归一化、乘 gamma、加 beta。

1. RMSNorm 不减均值，只计算平方均值的倒数平方根，再乘 weight。

1. CUDA 实现常用一个 Block 处理一行，线程分片读取 hidden 元素。

1. 用 Warp/Block 规约求 sum、sum of squares 或 variance。

1. 性能优化关注访存次数、向量化、规约效率、FP32 累加和与其他 elementwise 操作融合。

#### 八、总结

LayerNorm/RMSNorm 的核心是“按行规约 + 按元素写回”。能写清楚 sum、variance、rsqrt、gamma/beta，并解释为什么 RMSNorm 更省计算，就已经覆盖了大部分面试重点。高性能实现再进一步考虑向量化、融合、半精度输入和 FP32 累加。

### 手写 CUDA GEMM：朴素版、Shared Memory Tiling 与 Bank Conflict

> 原文：[手写 CUDA GEMM：朴素版、Shared Memory Tiling 与 Bank Conflict](https://hd9568.github.io/blog/cuda-gemm-naive-shared-tiling/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**从朴素矩阵乘法写起，逐步引入 Shared Memory Tiling，理解 GEMM 为什么需要数据复用以及如何处理 Bank Conflict。

#### 一、核心结论

GEMM 是 CUDA 手写算子核心题。

- GEMM 计算 `C = A * B`。

- 朴素版本每个线程计算一个 `C[row][col]`。

- 朴素版本会重复从 Global Memory 读取 A 和 B，数据复用差。

- Shared Memory Tiling 把 A、B 的小块缓存到 shared memory，多线程复用。

- Padding 可以缓解某些 shared memory Bank Conflict。

- 真正高性能 GEMM 还会涉及寄存器 tiling、向量化、双缓冲、Tensor Core。

#### 二、GEMM 定义

矩阵形状：

```text
A: M x K
B: K x N
C: M x N
```

计算：

```text
C[row][col] = sum(A[row][k] * B[k][col]), k = 0..K-1
```

#### 三、朴素版本

```cpp
__global__ void gemm_naive(const float* A, const float* B, float* C,
                           int M, int N, int K) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row < M && col < N) {
        float sum = 0.0f;
        // 每个线程独立计算一个 C[row][col]。
        for (int k = 0; k < K; ++k) {
            sum += A[row * K + k] * B[k * N + col];
        }
        C[row * N + col] = sum;
    }
}
```

启动：

```cpp
dim3 block(16, 16);
dim3 grid((N + block.x - 1) / block.x,
          (M + block.y - 1) / block.y);
gemm_naive<<<grid, block>>>(A, B, C, M, N, K);
```

#### 四、为什么朴素版本慢

朴素版本中，相邻线程会重复读取 A 或 B。

例如同一个 Block 计算 `C` 的一个 tile：

- 同一行的多个线程会读取相同的 `A[row][k]`。

- 同一列的多个线程会读取相同的 `B[k][col]`。

这些数据本可以复用，但朴素版本反复访问 Global Memory，带宽压力大。

#### 五、Shared Memory Tiling

Tiling 思路：

1. 把 A 的一个 tile 和 B 的一个 tile 从 Global Memory 读到 Shared Memory。

1. Block 内线程复用这两个 tile 做一部分累加。

1. 沿 K 维循环多个 tile，最终得到 C 的一个 tile。

```text
A tile: TILE x TILE
B tile: TILE x TILE
C tile: TILE x TILE
```

#### 六、Tiling 版本代码

```cpp
#define TILE 16
__global__ void gemm_tiled(const float* A, const float* B, float* C,
                           int M, int N, int K) {
    __shared__ float As[TILE][TILE];
    __shared__ float Bs[TILE][TILE];
    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int row = blockIdx.y * TILE + ty;
    int col = blockIdx.x * TILE + tx;
    float sum = 0.0f;
    // 沿 K 维分块。
    for (int t = 0; t < (K + TILE - 1) / TILE; ++t) {
        int a_col = t * TILE + tx;
        int b_row = t * TILE + ty;
        // 加载 A tile，越界位置补 0。
        if (row < M && a_col < K) {
            As[ty][tx] = A[row * K + a_col];
        } else {
            As[ty][tx] = 0.0f;
        }
        // 加载 B tile，越界位置补 0。
        if (b_row < K && col < N) {
            Bs[ty][tx] = B[b_row * N + col];
        } else {
            Bs[ty][tx] = 0.0f;
        }
        // 等待整个 tile 加载完成。
        __syncthreads();
        // 使用 shared memory 中的 tile 做局部乘加。
        #pragma unroll
        for (int k = 0; k < TILE; ++k) {
            sum += As[ty][k] * Bs[k][tx];
        }
        // 确保本轮 tile 使用完成，再进入下一轮覆盖 shared memory。
        __syncthreads();
    }
    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}
```

#### 七、Bank Conflict 与 Padding

如果访问 shared memory 的列方向，可能发生 Bank Conflict。可以加 padding：

```cpp
__shared__ float As[TILE][TILE + 1];
__shared__ float Bs[TILE][TILE + 1];
```

在上面的简单 GEMM 中，访问模式：

```cpp
As[ty][k]  // 同一行不同 k，通常较友好。
Bs[k][tx]  // 不同线程 tx 不同，通常也较友好。
```

但更复杂的 tile 布局、转置加载、Tensor Core layout 中，Bank Conflict 会更明显，Padding 或 Swizzle 会很重要。

#### 八、面试回答模板

如果问题是“怎么手写 GEMM”，可以这样回答：

1. 朴素版是一线程计算一个 `C[row][col]`，循环 K 维累加。

1. 朴素版会重复从 Global Memory 读取 A 和 B，数据复用差。

1. 优化版使用 Shared Memory Tiling，把 A 和 B 的小块加载到 shared memory。

1. Block 内线程复用 tile，减少 Global Memory 访问。

1. 每轮 tile 加载后需要 `__syncthreads()`，避免读到未完成数据。

1. 进一步优化包括 padding 避免 Bank Conflict、寄存器 tiling、向量化、双缓冲、Tensor Core。

#### 九、总结

GEMM 优化的核心是数据复用：

```text
Global Memory 读一次 -> Shared Memory 复用多次 -> 寄存器累加
```

朴素版用于理解映射，Shared Memory Tiling 用于理解复用，高性能版本则继续向寄存器 tiling 和 Tensor Core 演进。

## 6 GEMM、CUTLASS 与进阶优化

### Split-K 详解：从 GEMM K 维并行到 Attention Split-KV

> 原文：[Split-K 详解：从 GEMM K 维并行到 Attention Split-KV](https://hd9568.github.io/blog/cuda-split-k-gemm-attention/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**系统讲解 Split-K 如何用归约维并行补足 GPU 并行度，覆盖 Parallel/Serial/Atomic Split-K、Workspace 与 Epilogue、Split 数选择、Stream-K 区别，以及 Flash-Decoding 的 Split-KV Softmax 归并。

#### 一、Split-K 解决什么问题

GEMM：

```text
C[M, N] = A[M, K] @ B[K, N]
```

传统 GPU GEMM 主要沿输出矩阵的：

```text
M 维
N 维
```

划分 Thread Block。每个 CTA 独立计算一个 `C` Tile，并在 CTA 内串行遍历整个 K 维。

当 M、N 足够大时，输出 Tile 很多，GPU 并行度充足。

当：

```text
M 很小
N 很小或中等
K 很大
```

时会出现：

```text
输出 Tile 数少
-> CTA 数少
-> 大量 SM 空闲
-> 每个 CTA 却要串行扫描很长的 K
```

Split-K 的核心思路：

> 不只沿 M/N 划分输出，还把 K 归约维拆成多个片段，让多个 CTA 并行计算同一个输出 Tile 的部分和，再归并。

示意：

```text
普通 GEMM:
CTA 0:
  C_tile = A[:, 0:K] @ B[0:K, :]
Split-K, S = 4:
CTA 0: P0 = A[:, K0:K1] @ B[K0:K1, :]
CTA 1: P1 = A[:, K1:K2] @ B[K1:K2, :]
CTA 2: P2 = A[:, K2:K3] @ B[K2:K3, :]
CTA 3: P3 = A[:, K3:K4] @ B[K3:K4, :]
C_tile = P0 + P1 + P2 + P3
```

它用额外归并换取更多 CTA 并行度。

#### 二、普通 GEMM 如何映射到 GPU

设 CTA Tile：

```text
BLOCK_M x BLOCK_N x BLOCK_K
```

一个 CTA 负责输出：

```text
C[
  block_m * BLOCK_M : ...,
  block_n * BLOCK_N : ...
]
```

CTA 主循环：

```cpp
acc = 0;
for (int k = 0; k < K; k += BLOCK_K) {
    load A_tile[BLOCK_M, BLOCK_K];
    load B_tile[BLOCK_K, BLOCK_N];
    acc += A_tile @ B_tile;
}
store C_tile;
```

Grid 中 CTA 数：

```text
T_MN =
    ceil(M / BLOCK_M)
    * ceil(N / BLOCK_N)
```

普通 GEMM 的 K 循环发生在 CTA 内部，因此：

```text
K 再大
也不会增加 Grid 中 CTA 数
```

这正是 Split-K 要改变的地方。

#### 三、Tile Quantization 与 Wave Quantization

Split-K 的动机经常被笼统描述为“增加并行度”。更准确地说，它主要缓解两类量化问题。

##### 3.1 Tile Quantization

若：

```text
M % BLOCK_M != 0
N % BLOCK_N != 0
```

边界 Tile 只有部分元素有效，但 CTA 资源仍按完整 Tile 配置。

例如：

```text
M = 136
BLOCK_M = 128
```

需要两个 M Tile，第二个 Tile 只有 8 行有效。

这叫 Tile Quantization。

Split-K 通常不直接解决它，因为输出 Tile 形状没有变化。

##### 3.2 Wave Quantization

GPU 一次可并发运行有限数量 CTA。设：

```text
W =
    SM 数
    * 每个 SM 可驻留 CTA 数
```

若总 CTA 数为 `G`，理论 Wave 利用率：

```text
eta_wave =
    G
    /
    (ceil(G / W) * W)
```

例如：

```text
W = 4
G = 9
```

需要三波：

```text
4 + 4 + 1
```

理论利用率：

```text
9 / (3 * 4) = 75%
```

最后一波只有一个 SM 工作，其他 SM 等待。

Split-K 令：

```text
G_split = T_MN * S
```

其中 `S` 为 K Split 数。

若 `S = 2`：

```text
G_split = 18
eta_wave = 18 / (5 * 4) = 90%
```

但这只是并行度上限，还没有扣除归并与更短 K Slice 的效率损失。

#### 四、Split-K 的数学定义

GEMM：

```text
C = alpha * A B + beta * C_old
```

把 K 划分为 S 个不重叠区间：

```text
[k_0, k_1)
[k_1, k_2)
...
[k_{S-1}, k_S)
k_0 = 0
k_S = K
```

第 `s` 个 Partial：

```text
P_s[m, n] =
    sum_{k=k_s}^{k_{s+1}-1}
        A[m, k] * B[k, n]
```

完整结果：

```text
C[m, n] =
    alpha * sum_s P_s[m, n]
    + beta * C_old[m, n]
```

成立的原因是加法结合律在实数数学中成立。

浮点数中运算顺序变化会引入舍入误差，后文单独讨论。

##### 4.1 K 不一定能整除 S

可先按 K 元素均分：

```text
K_per_split = ceil(K / S)
k_begin = s * K_per_split
k_end = min(K, k_begin + K_per_split)
```

高性能 Kernel 通常按：

```text
BLOCK_K
MMA K Alignment
Scale Group
```

对齐，而不是随意切在任意元素位置。

#### 五、并行度为什么增加

普通 GEMM Grid：

```text
grid = [tiles_m, tiles_n]
```

Split-K Grid：

```text
grid = [tiles_m, tiles_n, split_k]
```

每个输出 Tile 原来只有一个 CTA：

```text
(tile_m, tile_n)
```

现在有 S 个 CTA：

```text
(tile_m, tile_n, split_id)
```

因此 CTA 数扩大 S 倍。

##### 5.1 典型有效 Shape

```text
M = 128
N = 128
K = 4096
BLOCK_M = 128
BLOCK_N = 128
```

普通 GEMM：

```text
T_MN = 1
只有 1 个 CTA
```

Split-K `S = 16`：

```text
16 个 CTA
每个 CTA 处理 K = 256
```

CUTLASS 官方文档使用的就是这一类例子。

##### 5.2 并行度不是免费增加

原来一个 CTA 在寄存器中完成：

```text
完整 C Tile
```

现在每个 CTA 只有：

```text
Partial C Tile
```

不同 CTA 不能直接读取彼此寄存器，因此 Partial 必须通过：

- Global Workspace。

- Output Buffer + Semaphore。

- Global Atomic。

- CTA Cluster/Remote Shared Memory。

之一完成归并。

#### 六、Parallel Split-K：两阶段归并

Parallel Split-K 让所有 K Slice 完全独立运行。

##### 6.1 第一阶段

Grid：

```text
[tiles_m, tiles_n, S]
```

第 `s` 个 CTA 写：

```text
workspace[
  s,
  tile_m,
  tile_n,
  BLOCK_M,
  BLOCK_N
]
```

逻辑上：

```text
workspace[s, :, :] = P_s
```

##### 6.2 第二阶段

Reduction Kernel：

```text
for each output element:
    acc = 0
    for s in [0, S):
        acc += workspace[s, m, n]
    C[m, n] =
        epilogue(alpha * acc + beta * C_old[m, n])
```

##### 6.3 特点

优点：

- 所有 Split 完全并行。

- 第一阶段没有同一输出 Tile 的 CTA 间等待。

- Reduction 顺序清晰。

- 容易支持 FP32 Partial。

代价：

- 需要 `S * M * N` 级别 Workspace。

- Partial 要写 HBM，再被 Reduction 读回。

- 多一次 Kernel Launch。

- Epilogue 必须延迟到第二阶段。

##### 6.4 Workspace

若 Partial 使用 FP32：

```text
workspace_bytes =
    S * M * N * 4
```

例子：

```text
M = N = 128
S = 16
```

Workspace：

```text
16 * 128 * 128 * 4
= 1,048,576 bytes
= 1 MiB
```

最终 BF16 输出只有：

```text
128 * 128 * 2 = 32 KiB
```

Partial Workspace 是最终输出的 32 倍。

#### 七、Serial Split-K：信号量顺序归并

Serial Split-K 仍让不同 K Slice 的 GEMM 主循环并行执行，但通过每个输出 Tile 的 Semaphore 顺序提交 Partial。

简化流程：

```text
Split 0:
  compute P0
  wait semaphore == 0
  workspace = P0
  semaphore = 1
Split 1:
  compute P1
  wait semaphore == 1
  workspace += P1
  semaphore = 2
...
Last Split:
  compute P_last
  wait semaphore == last_id
  total = workspace + P_last
  apply final epilogue
  write C
  semaphore = 0
```

##### 7.1 “Serial” 不表示 GEMM 主循环串行

各 CTA 可以并行计算自己的 K Slice：

```text
QK/MMA mainloop in parallel
```

串行的是：

```text
同一个输出 Tile 的 Partial Commit 顺序
```

如果 Split 0 很慢，后续 Split 即使先完成计算，也会在 Epilogue 前等待。

##### 7.2 Workspace

一种实现只需要：

```text
每个输出 Tile 一个 Partial Buffer
每个输出 Tile 一个 Semaphore
```

Workspace 不必乘 S。

代价是：

- 信号量轮转。

- 等待和 Cache Coherence。

- 同一输出 Tile 的归并顺序串行。

#### 八、Atomic Split-K

另一种方法是让每个 Split 直接：

```cpp
atomicAdd(&C[m, n], partial);
```

调用前先清零 C：

```text
C = 0
```

##### 8.1 优点

- 不需要完整 Partial Workspace。

- 不需要独立 Reduction Kernel。

- 实现简单。

##### 8.2 缺点

- 多个 Split 争用同一输出地址。

- Atomic 吞吐依赖 Dtype 和架构。

- 浮点累加顺序由调度决定，通常不确定。

- `beta * C_old` 难以直接融合。

- Bias、Activation 等 Epilogue 仍要单独处理。

- 大输出 Tile 上 Atomic 流量高。

Atomic 更适合：

- Split 数不大。

- 输出规模较小。

- 原子类型高效。

- 不要求严格确定性。

不能默认它比两阶段 Reduction 更快。

#### 九、Epilogue 为什么必须最后执行

GEMM Epilogue 常包含：

```text
alpha/beta
bias
activation
quantization
gating
residual
```

正确结果：

```text
y =
  activation(
      alpha * sum_s P_s
      + beta * C_old
      + bias
  )
```

错误做法：

```text
sum_s activation(
    alpha * P_s
    + beta * C_old
    + bias
)
```

因为 Activation 通常不满足线性：

```text
ReLU(x + y) != ReLU(x) + ReLU(y)
SiLU(x + y) != SiLU(x) + SiLU(y)
```

##### 9.1 Beta 也只能应用一次

若每个 Split 都计算：

```text
P_s + beta * C_old
```

最终会得到：

```text
sum_s P_s + S * beta * C_old
```

这是错误结果。

因此前 S-1 个 Split 通常只产生 Partial，最后归并完成后再执行完整 Epilogue。

#### 十、可运行的简化 CUDA 实现

下面的代码强调 Split-K 语义。它让一个线程计算一个 Partial 输出元素，没有 Shared Memory Tiling 或 Tensor Core，因此不代表高性能实现。

##### 10.1 Partial Kernel

```cpp
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <cstddef>
__global__ void split_k_partial(
    const half* A,        // [M, K], row-major
    const half* B,        // [K, N], row-major
    float* partials,      // [S, M, N]
    int M,
    int N,
    int K,
    int S) {
    const int n = blockIdx.x * blockDim.x + threadIdx.x;
    const int m = blockIdx.y * blockDim.y + threadIdx.y;
    const int split = blockIdx.z;
    if (m >= M || n >= N) {
        return;
    }
    const int k_per_split = (K + S - 1) / S;
    const int k_begin = split * k_per_split;
    const int k_end = min(K, k_begin + k_per_split);
    float acc = 0.0f;
    for (int k = k_begin; k < k_end; ++k) {
        const float a = __half2float(A[m * K + k]);
        const float b = __half2float(B[k * N + n]);
        acc = fmaf(a, b, acc);
    }
    const size_t offset =
        (static_cast<size_t>(split) * M + m) * N + n;
    partials[offset] = acc;
}
```

##### 10.2 Reduction Kernel

```cpp
__global__ void reduce_split_k(
    const float* partials,
    half* C,             // 既是 C_old，也是输出
    const half* bias,
    int M,
    int N,
    int S,
    float alpha,
    float beta) {
    const int n = blockIdx.x * blockDim.x + threadIdx.x;
    const int m = blockIdx.y * blockDim.y + threadIdx.y;
    if (m >= M || n >= N) {
        return;
    }
    float total = 0.0f;
    for (int split = 0; split < S; ++split) {
        const size_t offset =
            (static_cast<size_t>(split) * M + m) * N + n;
        total += partials[offset];
    }
    const int out_idx = m * N + n;
    float y = alpha * total;
    if (beta != 0.0f) {
        // beta == 0 时不能读取可能尚未初始化的 C。
        y += beta * __half2float(C[out_idx]);
    }
    if (bias != nullptr) {
        y += __half2float(bias[n]);
    }
    // 非线性必须在所有 Partial 归并后执行。
    y = fmaxf(y, 0.0f);
    C[out_idx] = __float2half_rn(y);
}
```

##### 10.3 Launch

```cpp
void launch_split_k(
    const half* A,
    const half* B,
    half* C,
    const half* bias,
    float* workspace,
    int M,
    int N,
    int K,
    int split_k,
    cudaStream_t stream) {
    dim3 block(16, 16);
    dim3 partial_grid(
        (N + block.x - 1) / block.x,
        (M + block.y - 1) / block.y,
        split_k);
    split_k_partial<<<partial_grid, block, 0, stream>>>(
        A, B, workspace, M, N, K, split_k);
    dim3 reduce_grid(
        (N + block.x - 1) / block.x,
        (M + block.y - 1) / block.y);
    reduce_split_k<<<reduce_grid, block, 0, stream>>>(
        workspace,
        C,
        bias,
        M,
        N,
        split_k,
        1.0f,
        0.0f);
}
```

高性能版本会把 `split_k_partial` 替换为：

- CTA Tiling。

- Shared Memory/TMA Pipeline。

- Tensor Core MMA。

- Vectorized Load。

- Swizzle。

- Warp Specialization。

但两阶段数据依赖与此代码相同。

#### 十一、CUTLASS 的官方实现模型

CUTLASS 官方文档把跨 CTA 的方案称为：

```text
parallel reduction splitK
```

实现包括两个 Kernel：

```text
Partitioned-K GEMM
-> Batched Reduction
```

设备级接口：

```text
cutlass::gemm::device::GemmSplitKParallel
```

官方例子：

```text
M = 128
N = 128
K = 4096
S = 16
```

得到 16 个：

```text
[128, 128, 256]
```

的 Partial GEMM。

##### 11.1 不整除

CUTLASS 文档还给出：

```text
K = 4096
S = 20
```

其中前 19 个 Partition 处理 204，最后一个处理 220。

实际 Kernel 要保证切分边界满足 MMA/Load Alignment，并正确处理 Tail。

##### 11.2 Workspace 所有权

CUTLASS 要求调用方提供存放 Partial 的 Workspace。

这意味着 Split-K 不能只看 Kernel 本身，还要把：

- Workspace 分配。

- 清零。

- 生命周期。

- CUDA Graph 地址稳定性。

纳入算子设计。

#### 十二、TensorRT-LLM 的 Parallel Split-K Grouped GEMM

TensorRT-LLM 的 `splitkGroupGemm` 是一个直接的 Parallel Split-K 例子。

##### 12.1 接口

```cpp
void splitkGroupedGemm(
    problem_sizes,
    ptrA,
    ptrB,
    ptrC,
    ptrD,
    params_workspace,
    gemm_workspace,
    split_k_slices,
    stream);
```

它同时处理多个不同 Shape 的 GEMM，并为每个 Problem 做 Split-K。

##### 12.2 Workspace 公式

源码按所有 Problem 的输出元素总数计算：

```cpp
workspace =
    sum_i(M_i * N_i)
    * sizeof(float)
    * split_k_slices;
```

Partial 使用 FP32，最终再转换为 FP16/BF16。

##### 12.3 第一阶段 Grid

```cpp
dim3 grid(
    threadblock_count,
    1,
    split_k_slices);
```

`grid.z` 就是 K Partition。

##### 12.4 第二阶段

Reduction Kernel：

```cpp
for each problem:
    for each output element:
        float sum = 0
        for split in [0, S):
            sum += partial[split, element]
        output[element] = cast(sum)
```

这条路径清晰体现：

```text
S 倍 Partial Workspace
两次 Kernel Launch
完全并行的第一阶段
```

#### 十三、LMDeploy 的 Serial Split-K

LMDeploy TurboMind GEMM 展示了另一种设计。

##### 13.1 Scheduler 切 K

Grid 第三维：

```cpp
tiles_[2] = splits;
```

Scheduler 把 K Chunk 尽量均匀分配给 Split：

```cpp
chunks = ceil(K / chunk_k);
base = chunks / splits;
remainder = chunks % splits;
```

每个 Split 的 Chunk 数最多相差 1。

##### 13.2 信号量

Epilogue 为每个输出 Tile 维护：

```text
barrier[tile_id]
```

第 `s` 个 Split：

```cpp
sem_wait(barrier, s);
```

保证按：

```text
0 -> 1 -> 2 -> ... -> S-1
```

归并。

##### 13.3 Partial Buffer

首个 Split：

```text
workspace = P0
```

中间 Split：

```text
workspace += Ps
```

最后 Split：

```text
total = workspace + P_last
-> alpha/beta
-> activation
-> output
```

##### 13.4 Workspace 不乘 S

该实现明确使用 Serial 模式：

```cpp
barriers_size =
    sizeof(int) * output_tiles;
partials_size =
    sizeof(float)
    * CTA_M * CTA_N
    * output_tiles;
```

Partial Workspace 只保存每个输出 Tile 的 Running Sum，不为每个 Split 单独分配一份。

这是：

```text
更小 Workspace
vs.
信号量等待与顺序提交
```

之间的取舍。

#### 十四、Split 数如何选择

`S` 是 Split-K 最重要的超参数。

##### 14.1 并行度下界

基础输出 Tile：

```text
T_MN =
    ceil(M / BLOCK_M)
    * ceil(N / BLOCK_N)
```

设备可同时驻留：

```text
W =
    num_sms
    * active_ctas_per_sm
```

若希望至少一波满载：

```text
S >= ceil(W / T_MN)
```

若希望有两波工作以隐藏 CTA 尾部：

```text
S >= ceil(2W / T_MN)
```

这只是并行度估算，不是最终最优值。

##### 14.2 K Slice 不能太短

设每次 Mainloop 消耗：

```text
BLOCK_K
```

Split 后每个 CTA 的 K Iteration：

```text
iters_per_split ≈
    ceil(K / BLOCK_K) / S
```

若只剩 1 到 2 次迭代：

- Pipeline 预热/排空占比高。

- TMA/Async Copy 难以隐藏。

- MMA 指令太少。

- Epilogue 占比高。

所以应限制：

```text
K_per_split >= K_min
```

`K_min` 取决于架构、Dtype 和 Kernel Pipeline。

##### 14.3 Workspace 上限

Parallel Split-K：

```text
S <=
  workspace_limit
  /
  (M * N * accumulator_bytes)
```

大 M/N 时，即使 K 很大，Workspace 也可能阻止使用较大 S。

##### 14.4 对齐

K Slice 边界通常要满足：

- Vector Load Alignment。

- Tensor Core MMA K。

- Quantization Group Size。

- Block Scale Vector Size。

量化 GEMM 尤其不能随意切断一个 Scale Group。

##### 14.5 实用启发式

```python
def choose_split_k(
    m, n, k,
    block_m, block_n, block_k,
    resident_ctas,
    max_splits,
    min_k_iters=4):
    output_tiles = ceil_div(m, block_m) * ceil_div(n, block_n)
    if output_tiles >= 2 * resident_ctas:
        return 1
    splits_for_parallelism = ceil_div(
        2 * resident_ctas,
        output_tiles,
    )
    total_k_iters = ceil_div(k, block_k)
    splits_for_pipeline = max(
        1,
        total_k_iters // min_k_iters,
    )
    return max(
        1,
        min(
            splits_for_parallelism,
            splits_for_pipeline,
            max_splits,
        ),
    )
```

实际库还会比较多个候选的实测时间。

#### 十五、性能模型与 Workspace 代价

粗略时间：

```text
T_split =
    T_partial_gemm
    + T_partial_write
    + T_reduction_read
    + T_reduction
    + T_extra_launch
```

收益来自：

```text
T_partial_gemm
```

因并行度增加而下降。

代价来自后四项。

##### 15.1 FLOPs 没有减少

总 GEMM FLOPs：

```text
2 * M * N * K
```

Split-K 不减少数学计算量，只改变分工。

##### 15.2 输入流量

各 Split 读取不重叠 K 区间，理想情况下 A/B 的算法级总字节量不会因 S 直接乘 S。

但实际可能恶化：

- 每个 Slice 重新执行 Prologue。

- Cache Reuse 变化。

- 边界对齐和 Padding。

- 更短 Pipeline。

- 更大的 Grid 调度开销。

##### 15.3 输出流量

Parallel Split-K 额外产生：

```text
Partial Write:
  S * M * N * accumulator_bytes
Partial Read:
  S * M * N * accumulator_bytes
```

约：

```text
2 * S * M * N * accumulator_bytes
```

的 Workspace 流量。

当 K 不够大时，这些流量会抵消并行收益。

#### 十六、Split-K 与 Sliced-K 的区别

两者都切 K，但切分层级不同。

##### Split-K

```text
跨 CTA 切 K
```

特点：

- 增加 Grid CTA 数。

- 能占用更多 SM。

- 需要跨 CTA 归并。

- 通常经过 Global Memory/Atomic/Cluster。

##### Sliced-K

```text
同一 CTA 内，不同 Warp 切 K
```

特点：

- 不增加 CTA 数。

- 增加 CTA 内 Warp 并行。

- Partial 在 CTA 内归并。

- 可通过 Shared Memory/Warp Shuffle。

- 适合 M/N 小、CTA 内 Warp 利用不足。

Sliced-K 不能解决：

```text
整个 Grid 只有几个 CTA
```

导致的跨 SM 并行不足。

#### 十七、Split-K 与 Stream-K 的区别

Split-K 使用固定切分数：

```text
每个输出 Tile -> S 个 K Slice
```

问题是：

- 所有 Tile 都被切同样多份。

- 可能产生过多 Partial。

- 最后一波仍可能不满。

- S 需要按 Shape 调优。

##### 17.1 Stream-K

Stream-K 不固定“每个 Tile 几个 Split”，而是把整个 GEMM 的：

```text
MAC Mainloop Iterations
```

尽量均匀分配给固定数量的 Persistent CTA。

设所有输出 Tile 一共需要：

```text
total_k_iters
```

Stream-K 让每个 CTA 获得接近：

```text
total_k_iters / num_ctas
```

的工作。

一个 CTA 的工作可能：

- 从某个 Tile 中间开始。

- 完成该 Tile。

- 继续处理下一个完整 Tile。

- 在另一个 Tile 中间结束。

只有被多个 CTA 共同处理的 Tile 需要 Partial 归并。

##### 17.2 核心差异

| 维度 | Split-K | Stream-K |
| --- | --- | --- |
| 切分单位 | 每个输出 Tile 的固定 K Slice | 全局 MAC Loop Iteration |
| CTA 数 | `output_tiles * S` | 通常接近指定 Persistent Grid |
| 负载均衡 | Shape 依赖 | 更均匀 |
| Partial 数 | 可能很多 | 只在 CTA 边界共享 Tile |
| 调度复杂度 | 较低 | 较高 |
| 主要参数 | Split 数 S | Persistent CTA 数/策略 |

Stream-K 论文的核心目标是缓解 Wave Quantization，并减少对大量 Tile 配置和 Split 启发式的依赖。

#### 十八、Attention 中的 Split-KV

Attention Decode：

```text
O =
  softmax(Q K_cache^T * scale)
  V_cache
```

此时：

```text
Q_len 通常为 1
KV_len 可能很长
```

若每个请求/Head 只启动一个 CTA：

```text
CTA 数 ≈ batch * query_heads
```

Batch 较小时可能远少于 SM 数。

Flash-Decoding 增加：

```text
KV Sequence Split
```

并行度变为近似：

```text
batch * query_heads * num_kv_splits
```

这通常称为：

- Split-KV。

- Split-K Attention。

- Multi-CTA KV。

- Flash-Decoding。

##### 18.1 它不是 GEMM Head-Dim Split-K

Attention 的 `K` 同时可能指：

- Key Tensor。

- GEMM Reduction Dimension。

Decode Split-KV 通常切的是：

```text
KV Sequence Length
```

不是 Q/K Head Dimension。

每个 Split 读取一段历史 Token 的 K/V。

#### 十九、Split-KV 的 Softmax 正确归并

GEMM Partial 可以直接求和：

```text
C = sum_s P_s
```

Attention 不行：

```text
softmax([scores_0, scores_1])
```

不等于两个局部 Softmax 输出直接相加或平均。

##### 19.1 每个 Split 的状态

第 `s` 个 KV Segment 的 Score：

```text
x_{s,j} = q k_{s,j}^T * scale
```

局部最大值：

```text
m_s = max_j x_{s,j}
```

局部分母：

```text
l_s =
  sum_j exp(x_{s,j} - m_s)
```

局部未归一化输出：

```text
o_s =
  sum_j exp(x_{s,j} - m_s) v_{s,j}
```

##### 19.2 全局归并

全局最大值：

```text
m = max_s m_s
```

修正系数：

```text
alpha_s = exp(m_s - m)
```

全局分母：

```text
l =
  sum_s alpha_s * l_s
```

全局分子：

```text
o =
  sum_s alpha_s * o_s
```

最终输出：

```text
O = o / l
```

##### 19.3 使用 LSE 表示

局部 LogSumExp：

```text
LSE_s = m_s + log(l_s)
```

全局：

```text
LSE =
  logsumexp_s(LSE_s)
```

若局部输出已经归一化：

```text
O_s = o_s / l_s
```

则：

```text
O =
  sum_s exp(LSE_s - LSE) * O_s
```

Flash-Decoding 就是保存：

```text
Partial Output + LSE
```

再执行第二阶段归并。

##### 19.4 简化合并代码

```python
def merge_attention_splits(partial_out, partial_lse):
    # partial_out: [S, H, D]
    # partial_lse: [S, H]
    global_lse = torch.logsumexp(partial_lse, dim=0)
    weights = torch.exp(
        partial_lse - global_lse.unsqueeze(0)
    )
    output = torch.sum(
        weights.unsqueeze(-1) * partial_out,
        dim=0,
    )
    return output, global_lse
```

实际 Kernel 使用 FP32 统计量，并避免显式构造大 Weight Tensor。

#### 二十、Paged KV、GQA 与 Blackwell

##### 20.1 Paged KV

KV Segment 不一定对应连续物理地址。

每个 Split 需要：

```text
logical token range
-> logical page
-> block table
-> physical KV page
```

Split 边界最好与 Page/Tile 对齐，减少：

- 首尾 Mask。

- Page Table 查询。

- 非合并访问。

##### 20.2 GQA

GQA：

```text
group_size =
  num_query_heads / num_kv_heads
```

同一 KV Head 服务多个 Query Head。

高效 Kernel 会让一个 CTA：

- 读取一次 K/V Tile。

- 为多个 Query Head 计算 Score/Output。

如果 Split-KV 调度只按 Query Head 独立读取 KV，会丢失 GQA 的复用收益。

因此 Split 数不能只看：

```text
KV_len / chunk_size
```

还要看：

- Query Head Group。

- Batch。

- KV Head。

- 每 CTA 处理的 Head 数。

- SM 数。

##### 20.3 Blackwell

Split-K 思想与架构无关，但 Blackwell 改变了取舍。

Blackwell Kernel 可使用：

- TMA。

- TMEM。

- UMMA/tcgen05。

- CTA Cluster。

- Persistent Scheduler。

大 Tile 更高效，却会减少 M/N Output Tile 数，因此在部分 Shape 上更需要额外并行维度。

归并可以选择：

- Global Workspace。

- Separate Reduction Kernel。

- Cluster Remote Shared Memory。

TensorRT-LLM Blackwell FMHA 的 Multi-CTA KV 就包含 Global Reduction 和 CGA Shared Memory Reduction 路径。

但不能仅因 GPU 是 Blackwell 就固定增加 Split 数。Cluster Occupancy、TMEM/SMEM 和 Reduction 成本仍需实际调优。

#### 二十一、数值稳定性与确定性

##### 21.1 GEMM 浮点顺序

数学上：

```text
(a + b) + c = a + (b + c)
```

浮点中不一定成立。

Split-K 改变：

- K 分组。

- Partial 内累加顺序。

- Partial 间归并顺序。

所以结果可能与非 Split-K 有微小差异。

##### 21.2 Parallel Reduction

若 Reduction 顺序固定，通常可以做到同配置重复运行一致，但不保证与普通 GEMM Bitwise 相同。

##### 21.3 Serial Reduction

Semaphore 固定 Split 顺序：

```text
0 -> 1 -> ... -> S-1
```

确定性通常更容易保证。

##### 21.4 Atomic

Atomic 到达顺序依赖 CTA 调度，通常不保证 Bitwise Deterministic。

##### 21.5 Attention

Split-KV 必须：

- Max/Sum 使用 FP32。

- 先做局部 Max Subtraction。

- 用 `exp(m_s - m)` 重标定。

- 正确处理全 Mask Segment。

- 避免 `-inf - -inf` 产生 NaN。

不能直接累加局部归一化输出。

#### 二十二、何时使用，何时不要使用

##### 适合 Split-K

- M/N 小，K 大。

- Output Tile 数少于 SM 并发容量。

- Wave Quantization 严重。

- LLM Decode 小 Batch Linear。

- LoRA 小 N 或小 M GEMM。

- MoE 中单 Expert Token 很少但 K 很大。

- Long-context Decode Attention。

##### 不适合 Split-K

- M/N 已产生多波 CTA。

- K 很小。

- 每个 K Slice 只有少量 Mainloop Iteration。

- Parallel Workspace 过大。

- Reduction 已占主要时间。

- 强确定性要求但只能用 Atomic。

- Epilogue 很复杂且难以延后。

- GEMV 极度 Memory-bound，额外 Partial 流量超过并行收益。

##### 22.1 Split-K 不是默认越大越好

典型性能曲线：

```text
S = 1:
  并行不足
S 适中:
  SM 利用率提高
  性能最佳
S 过大:
  K Slice 太短
  Workspace/Reduction/Launch 主导
  性能下降
```

应该把 `S` 视为 Kernel 配置的一部分，而不是模型常量。

#### 二十三、Profile 与调优清单

##### 23.1 Nsight Systems

检查：

- Partial GEMM 与 Reduction 是否为两次 Kernel。

- Reduction Kernel 占总时间比例。

- 是否存在 Workspace 清零。

- CTA Wave 是否更完整。

- Serial Split 是否出现明显等待。

- CUDA Graph 是否包含 Workspace 地址变化。

##### 23.2 Nsight Compute

Partial GEMM：

```text
SM Active
Tensor Core Utilization
Eligible Warps
Achieved Occupancy
DRAM Throughput
L2 Hit Rate
Mainloop Iterations
```

Reduction：

```text
DRAM Read/Write
Atomic Throughput
Memory Coalescing
FP32 Pipe
Kernel Launch Duration
```

##### 23.3 必须比较的候选

```text
S = 1, 2, 4, 8, 16, ...
```

同时比较：

- 不同 BLOCK_M/N/K。

- Parallel vs Serial。

- Split-K vs Smaller Output Tile。

- Split-K vs Stream-K/Persistent。

- FP16/BF16/FP8。

- 不同 M/N/K。

##### 23.4 记录完整成本

不要只测 Partial GEMM：

```text
total =
  workspace_prepare
  + partial_gemm
  + reduction
  + epilogue
```

Attention 还要包括：

```text
block table
partial stats
LSE merge
output write
```

#### 二十四、参考资料

1. [NVIDIA CUTLASS：Efficient GEMM in CUDA - Parallelized Reductions](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/efficient_gemm.html#parallelized-reductions)

1. [NVIDIA CUTLASS GEMM API](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/gemm_api.html)

1. [NVIDIA Matrix Multiplication Background Guide](https://docs.nvidia.com/deeplearning/performance/dl-performance-matrix-multiplication/index.html)

1. [Stream-K: Work-centric Parallel Decomposition for Dense GEMM](https://arxiv.org/abs/2301.03598)

1. [CUTLASS Tutorial: Persistent Kernels and Stream-K](https://research.colfax-intl.com/cutlass-tutorial-persistent-kernels-and-stream-k/)

1. [Flash-Decoding for Long-Context Inference](https://crfm.stanford.edu/2023/10/12/flashdecoding.html)

1. [FlashInfer：Attention States and Recursive Attention](https://docs.flashinfer.ai/tutorials/recursive_attention.html)

1. [cuBLASLt Reduction Scheme](https://docs.nvidia.com/cuda/nvmath-python/0.3.0/bindings/generated/nvmath.bindings.cublasLt.ReductionScheme.html)

#### 二十五、总结

GEMM Split-K 的主线：

```text
普通 GEMM:
  一个输出 Tile由一个 CTA 扫描完整 K
Split-K:
  多个 CTA 扫描不同 K Slice
  -> 产生 Partial
  -> 归并
  -> 最后执行 Epilogue
```

它的收益来自：

```text
增加 CTA 数
减少 SM 空闲
缓解 Wave Quantization
```

它的代价是：

```text
Workspace
Partial HBM 流量
Reduction
同步
额外 Kernel Launch
浮点顺序变化
```

三种主要实现：

```text
Parallel Split-K:
  S 份 Workspace + 独立 Reduction
Serial Split-K:
  单份 Running Partial + Semaphore 顺序提交
Atomic Split-K:
  直接 Atomic 到输出，限制最多
```

与相关技术的边界：

```text
Sliced-K:
  CTA 内 Warp 切 K
Stream-K:
  全局均分 MAC Iteration，减少固定 Split 的量化问题
Attention Split-KV:
  切 KV Sequence，并用 Max/Sum/LSE 规则归并 Softmax 状态
```

最终判断标准不是“是否使用 Split-K”，而是：

```text
增加的并行收益
是否大于
归并、Workspace 和 Pipeline 变短的代价
```

### CUTLASS 入门：从 CUDA Tiling 到可组合 GEMM Kernel

> 原文：[CUTLASS 入门：从 CUDA Tiling 到可组合 GEMM Kernel](https://hd9568.github.io/blog/cuda-cutlass-from-template-to-gemm/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**面向掌握基础 CUDA 的读者，从 C++ 模板、Layout 和 GEMM 分块开始，通过多个递进 Demo 讲清 CUTLASS 2.x Device API、3.x Collective API、Mainloop、Epilogue、流水线及调优方法。

#### 一、CUTLASS 到底是什么

CUTLASS 是 NVIDIA 提供的开源 CUDA C++ 模板库，重点是高性能 GEMM 及其衍生算子。

GEMM 的完整形式为：

```text
D = alpha * A * B + beta * C
A: [M, K]
B: [K, N]
C: [M, N]
D: [M, N]
```

cuBLAS 与 CUTLASS 都能计算 GEMM，但使用方式不同：

```text
cuBLAS:
  调用 NVIDIA 已编译好的库函数。
  使用简单，但很难改写 Kernel 内部结构。
CUTLASS:
  在编译期组合数据类型、Layout、Tile、数据搬运、
  MMA、流水线和 Epilogue，生成应用自己的 CUDA Kernel。
```

因此 CUTLASS 不是一门新语言。它仍是 CUDA C++，只是把高性能 Kernel 中反复出现的组件封装成模板。

##### 从手写 CUDA GEMM 到 CUTLASS

基础 CUDA Tiled GEMM 通常是：

```text
Global Memory
-> A/B Tile 搬到 Shared Memory
-> __syncthreads()
-> 每个线程在寄存器中累加
-> 写回 C
```

CUTLASS 继续细化这条路径：

```text
Problem
-> CTA Tile
-> Warp / Warpgroup Tile
-> MMA Instruction Tile
-> Register Accumulator
-> Epilogue
-> Global Memory
```

它替你处理的不是矩阵乘法公式，而是以下工程细节：

- 不同线程怎样协作搬运 Tile。

- Shared Memory 怎样布局以减少 Bank Conflict。

- Warp 怎样拥有 MMA Fragment。

- 如何使用 `mma.sync`、WGMMA 或 `tcgen05.mma`。

- 如何双缓冲并重叠数据搬运与计算。

- 边界 Tile 如何 Predication。

- Accumulator 怎样重排后合并写回。

可以把 CUTLASS 理解为：

> 一套把 GEMM 算法映射到 NVIDIA GPU 层级结构的可组合零开销抽象。

“零开销”不是说 Kernel 没有成本，而是模板抽象在编译期展开，运行时不会因为 C++ 类和模板多出虚函数或解释器开销。

#### 二、读懂 CUTLASS 前需要的 C++ 模板知识

CUTLASS 代码长，主要因为大量选择被编码进“类型”。先看一个缩小版例子。

##### Demo 0：类型也能作为参数

```cpp
#include <cstdio>
#include <type_traits>
struct Simt {};
struct TensorOp {};
template <typename MathTag, int TileM, int TileN>
struct KernelConfig {
  static void print() {
    if constexpr (std::is_same_v<MathTag, TensorOp>) {
      std::printf("Tensor Core, tile = %d x %d\n", TileM, TileN);
    } else {
      std::printf("CUDA Core, tile = %d x %d\n", TileM, TileN);
    }
  }
};
using MyKernel = KernelConfig<TensorOp, 128, 128>;
int main() {
  MyKernel::print();
}
```

这里有四个关键概念。

##### `template`：把配置变成编译期输入

```cpp
template <typename MathTag, int TileM, int TileN>
```

表示模板接收一个类型和两个整数。编译器看到：

```cpp
KernelConfig<TensorOp, 128, 128>
```

会为这组配置实例化具体代码。`TileM/TileN` 已是编译期常量，循环展开、数组大小和指令选择都可以据此优化。

##### `using`：给复杂类型取别名

```cpp
using MyKernel = KernelConfig<TensorOp, 128, 128>;
```

CUTLASS 中几十行 `using` 通常不是在执行计算，而是在逐层组装 Kernel 类型。

##### Tag：空类型也能表达选择

`TensorOp` 没有字段，却能告诉模板选择 Tensor Core 路径。CUTLASS 常见 Tag 包括：

```cpp
cutlass::arch::OpClassSimt
cutlass::arch::OpClassTensorOp
cutlass::arch::Sm80
cutlass::layout::RowMajor
```

它们把“选择什么实现”放进类型系统。

##### `if constexpr`：只保留命中的分支

普通 `if` 两条分支都必须是合法运行时代码；`if constexpr` 在编译期选择分支，未选分支不会进入最终 Kernel。

这也是 CUTLASS 编译慢、报错长的原因：编译器需要实例化深层模板。但收益是，运行时得到的是已经特化好的 CUDA 代码。

#### 三、Demo 1：调用最小 CUTLASS GEMM

先不自己配置 Tile，直接使用 `cutlass::gemm::device::Gemm` 的默认值。

```cpp
#include <cuda_runtime.h>
#include "cutlass/cutlass.h"
#include "cutlass/gemm/device/gemm.h"
using ColumnMajor = cutlass::layout::ColumnMajor;
using CutlassGemm = cutlass::gemm::device::Gemm<
    float, ColumnMajor,   // A 的元素类型和布局
    float, ColumnMajor,   // B
    float, ColumnMajor>;  // C、D
cutlass::Status run_gemm(
    int M, int N, int K,
    float const* A,
    float const* B,
    float const* C,
    float* D) {
  int lda = M;
  int ldb = K;
  int ldc = M;
  int ldd = M;
  float alpha = 1.0f;
  float beta = 0.0f;
  CutlassGemm gemm;
  CutlassGemm::Arguments args(
      {M, N, K},
      {A, lda},
      {B, ldb},
      {C, ldc},
      {D, ldd},
      {alpha, beta});
  return gemm(args);
}
```

这是一个 Host 函数。代码中没有手写 `<<<grid, block>>>`，但 `gemm(args)` 内部最终仍会启动 CUDA Kernel。

##### 每个参数对应什么

```text
{M, N, K}
  GEMM 的逻辑形状。
{A, lda}
  A 的设备指针和 Leading Dimension。
{alpha, beta}
  Epilogue 中 D = alpha * accumulator + beta * C。
```

Column-major 的地址公式为：

```text
offset(row, col) = row + col * leading_dimension
```

所以：

```text
A[M,K] 的 lda = M
B[K,N] 的 ldb = K
C[M,N] 的 ldc = M
```

如果 Layout 和 `ld*` 不匹配，程序通常不会崩溃，而是得到错误矩阵。这是学习 CUTLASS 时最常见的正确性问题之一。

##### 这段代码何时决定什么

编译期决定：

```text
float
ColumnMajor
默认 CTA/Warp Tile
默认 Mainloop 和 Epilogue
目标架构对应实现
```

运行时决定：

```text
M、N、K
A/B/C/D 指针
Leading Dimension
alpha、beta
CUDA Stream
```

因此一份已编译 Kernel 可以处理多个 `M/N/K`，但不能突然把 `float` 改成 `half`，因为数据类型属于模板参数。

##### 编译

假设 CUTLASS 仓库位于 `$CUTLASS_DIR`：

```bash
nvcc -std=c++17 -O3 \
  -I"$CUTLASS_DIR/include" \
  -arch=sm_80 \
  demo.cu -o demo
```

CUTLASS 大部分是 Header-only。`nvcc` 在编译你的 `.cu` 时实例化模板，并把选中的设备代码编译进可执行文件。

#### 四、CUTLASS 内部怎样执行 GEMM

假设：

```text
M = N = K = 256
CTA Tile  = 128 x 128 x 32
Warp Tile =  64 x  64 x 32
MMA Tile  =  16 x   8 x 16
```

##### CTA 如何覆盖输出

一个 CTA 计算一个 `128 x 128` 的 D Tile：

```text
grid_m = ceil(256 / 128) = 2
grid_n = ceil(256 / 128) = 2
总 CTA 数 = 2 x 2 = 4
```

每个 CTA 沿 K 维循环：

```text
K iterations = 256 / 32 = 8
```

逻辑过程：

```text
for each CTA output tile (cta_m, cta_n):
    accumulator = 0
    for cta_k in [0, 32, ..., 224]:
        load A[cta_m : cta_m+128, cta_k : cta_k+32]
        load B[cta_k : cta_k+32, cta_n : cta_n+128]
        accumulator += A_tile @ B_tile
    D_tile = alpha * accumulator + beta * C_tile
```

##### Warp 和 MMA 如何继续切分

一个 `128 x 128` CTA Tile 可以由 4 个 Warp Tile 覆盖：

```text
Warp 0 -> C[0:64,    0:64]
Warp 1 -> C[0:64,   64:128]
Warp 2 -> C[64:128,  0:64]
Warp 3 -> C[64:128, 64:128]
```

每个 Warp 再发出多条 `16 x 8 x 16` MMA 指令。MMA 指令不是一个线程计算整个小矩阵，而是一个 Warp 的线程共同持有 A/B Fragment 和 Accumulator Fragment。

层级可以记成：

```text
Grid:
  把整个 [M,N] 输出分给 CTA
CTA:
  从 Global Memory 搬 A/B Tile 到 Shared Memory
Warp:
  从 Shared Memory 取自己的 Fragment
MMA:
  用 Tensor Core 更新 Register Accumulator
```

##### Mainloop 与 Epilogue

CUTLASS 把 GEMM 分成两大阶段。

Mainloop：

```text
沿 K 维循环
-> 搬运 A/B
-> 执行 MMA
-> 结果累加在寄存器
```

Epilogue：

```text
读取寄存器 Accumulator
-> 与 C、alpha、beta 或 Bias 等组合
-> 重排为合并访存
-> 写回 D
```

Epilogue 独立存在有两个原因：

1. MMA 的寄存器 Fragment 布局服务于计算，不一定适合 Global Memory 合并写。

1. `Bias + ReLU` 等操作可以在写回前融合，避免额外 Kernel 和显存往返。

#### 五、Demo 2：指定 Tensor Core、Tile 与 Epilogue

下面展示 CUTLASS 2.x 风格的完整配置骨架。它比 Demo 1 多做一件事：不再依赖默认配置。

```cpp
#include "cutlass/cutlass.h"
#include "cutlass/gemm/device/gemm.h"
#include "cutlass/epilogue/thread/linear_combination.h"
using ElementA = cutlass::half_t;
using ElementB = cutlass::half_t;
using ElementC = float;
using Accumulator = float;
using LayoutA = cutlass::layout::RowMajor;
using LayoutB = cutlass::layout::ColumnMajor;
using LayoutC = cutlass::layout::RowMajor;
using Epilogue = cutlass::epilogue::thread::LinearCombination<
    ElementC,
    128 / cutlass::sizeof_bits<ElementC>::value,
    Accumulator,
    float>;
using TensorCoreGemm = cutlass::gemm::device::Gemm<
    ElementA, LayoutA,
    ElementB, LayoutB,
    ElementC, LayoutC,
    Accumulator,
    cutlass::arch::OpClassTensorOp,
    cutlass::arch::Sm80,
    cutlass::gemm::GemmShape<128, 128, 32>,  // CTA Tile
    cutlass::gemm::GemmShape<64, 64, 32>,    // Warp Tile
    cutlass::gemm::GemmShape<16, 8, 16>,     // MMA Tile
    Epilogue>;
```

这段 `using` 本身不执行 GEMM，而是生成一个具体的 Kernel 类型。

##### 为什么输入 FP16、累加 FP32

Tensor Core 读取 FP16：

```text
A/B storage = FP16
```

但 K 维有很多次乘加。如果也用 FP16 Accumulator，舍入误差会快速累积，因此常用：

```text
FP16 x FP16 -> FP32 accumulate
```

最后 Epilogue 再按 `ElementC` 转换输出。

##### Alignment 为什么重要

Tensor Core Kernel 常使用 128-bit 向量化 Load：

```text
128 bits / 16 bits per FP16 = 8 elements
```

因此指针、Leading Dimension 和 K 维通常要满足相应对齐。模板实例可以编译成功，但运行时 Shape 不满足 `can_implement()` 时应拒绝执行：

```cpp
TensorCoreGemm gemm;
TensorCoreGemm::Arguments args = /* ... */;
if (gemm.can_implement(args) != cutlass::Status::kSuccess) {
  // 改用更弱对齐的 Kernel，或对输入 Padding。
}
```

不要把对齐只理解成“指针地址是 16 的倍数”。对二维 Tensor，下一行的起始地址也由 Leading Dimension 决定。

##### 为什么 Tile 不能越大越好

增大 CTA Tile 可以提高数据复用，但也会增加：

```text
Shared Memory
Register Accumulator
每 CTA 的 Warp 数
边界浪费
```

寄存器和 Shared Memory 过多时，每个 SM 同时驻留的 CTA 变少。最终性能取决于数据复用、并行 CTA 数、流水线深度和问题形状的平衡。

#### 六、Demo 3：理解 CUTLASS 3.x Collective API

Hopper 以后，数据搬运和计算方式明显复杂：TMA、WGMMA、Warp Specialization、Threadblock Cluster 都需要组合。CUTLASS 3.x 因而把 GEMM 拆成两个 Collective。

```text
CollectiveMainloop:
  A/B 如何从 Global Memory 到 Shared Memory
  如何形成流水线
  使用哪种 MMA
CollectiveEpilogue:
  Accumulator 如何转换、融合和写回
```

下面是官方 Hopper Quick Start 的核心骨架：

```cpp
#include "cute/tensor.hpp"
#include "cutlass/gemm/collective/collective_builder.hpp"
#include "cutlass/gemm/device/gemm_universal_adapter.h"
#include "cutlass/gemm/kernel/gemm_universal.hpp"
using namespace cute;
using ElementA = cutlass::half_t;
using ElementB = cutlass::half_t;
using ElementC = cutlass::half_t;
using Accumulator = float;
using LayoutA = cutlass::layout::RowMajor;
using LayoutB = cutlass::layout::ColumnMajor;
using LayoutC = cutlass::layout::ColumnMajor;
constexpr int AlignmentA = 128 / cutlass::sizeof_bits<ElementA>::value;
constexpr int AlignmentB = 128 / cutlass::sizeof_bits<ElementB>::value;
using ArchTag       = cutlass::arch::Sm90;
using OperatorClass = cutlass::arch::OpClassTensorOp;
using TileShape     = Shape<_128, _128, _64>;
using ClusterShape  = Shape<_1, _2, _1>;
using Mainloop = typename cutlass::gemm::collective::CollectiveBuilder<
    ArchTag, OperatorClass,
    ElementA, LayoutA, AlignmentA,
    ElementB, LayoutB, AlignmentB,
    Accumulator,
    TileShape, ClusterShape,
    cutlass::gemm::collective::StageCountAuto,
    cutlass::gemm::collective::KernelScheduleAuto
>::CollectiveOp;
```

`CollectiveBuilder` 根据这些约束选择合法实现。`Auto` 不是运行时自动调优，而是编译期规则选择。

然后组装 Epilogue 和完整 Kernel：

```cpp
using Epilogue = cutlass::epilogue::collective::DefaultEpilogue<
    cutlass::gemm::TagToStrideC_t<LayoutC>,
    cutlass::gemm::TagToStrideC_t<LayoutC>,
    cutlass::epilogue::thread::LinearCombination<
        ElementC, 1, Accumulator, Accumulator>>;
using GemmKernel = cutlass::gemm::kernel::GemmUniversal<
    Shape<int, int, int>,
    Mainloop,
    Epilogue>;
using Gemm = cutlass::gemm::device::GemmUniversalAdapter<GemmKernel>;
```

最后的职责链为：

```text
GemmUniversalAdapter
  Host 侧参数检查、Workspace、初始化和 Launch
GemmUniversal
  Grid/CTA 级 Kernel 骨架和 Tile Scheduler
Mainloop
  A/B 搬运、Pipeline、MMA
Epilogue
  alpha/beta、类型转换和 D 写回
```

##### 为什么 3.x 代码反而更长

2.x 的 `device::Gemm` 隐藏了大部分默认选择，适合直接调用。3.x 暴露了 Mainloop 与 Epilogue 的组合边界，适合：

- 更换 TMA/WGMMA Schedule。

- 使用 Persistent Kernel。

- 配置 Threadblock Cluster。

- 自定义融合 Epilogue。

- 在新架构上复用数据搬运与计算组件。

初学时应先理解 2.x 调用和内部层级，再读 3.x Builder。直接从数十个模板参数开始，容易只记住类型名而没有执行模型。

#### 七、编译、验证、调优与学习路线

##### 先用 Profiler 找到可用 Kernel

构建：

```bash
git clone --branch v4.7.1 https://github.com/NVIDIA/cutlass.git
cd cutlass
cmake -S . -B build \
  -DCUTLASS_NVCC_ARCHS=80 \
  -DCUTLASS_ENABLE_TESTS=OFF
cmake --build build --target cutlass_profiler -j
```

搜索并测试 GEMM：

```bash
./build/tools/profiler/cutlass_profiler \
  --operation=Gemm \
  --m=4096 --n=4096 --k=4096 \
  --A=f16:row --B=f16:column --C=f32:row
```

Profiler 会验证结果并输出 Kernel 名、Tile、Stage 和耗时。实践中应先让 Profiler 找到高性能配置，再把相近配置写入代码，而不是靠直觉猜 Tile。

##### 正确测量

至少检查：

```text
1. 与 cuBLAS 或 CPU/PyTorch Reference 比较数值。
2. Warmup 后用 CUDA Event 计时。
3. 计时区间外完成内存分配和初始化。
4. 使用实际业务 Shape，而不只测 4096^3。
5. 分开检查 Kernel 时间和端到端时间。
```

性能不佳时依次检查：

```text
是否真的选中 Tensor Core Kernel
M/N/K 与指针是否满足 Alignment
CTA 数是否足够占满 GPU
Tile 是否与长宽比匹配
Shared Memory/Register 是否限制 Occupancy
Epilogue 是否成为瓶颈
是否需要 Split-K 或 Persistent Schedule
```

##### CUTLASS 应放在什么位置

| 目标 | 优先选择 |
| --- | --- |
| 只需要标准 GEMM | cuBLAS/cuBLASLt |
| 需要已实现的 CUTLASS Kernel | `device::Gemm` 或 Operator API |
| 要组合 Hopper/Blackwell Mainloop | CUTLASS 3.x Collective |
| 要直接控制 Layout、Copy、MMA | CuTe C++ / CuTe DSL |
| 要快速写普通逐元素或归约 Kernel | CUDA C++ 或 Triton 通常更直接 |

CUTLASS 最重要的学习成果不是记住模板签名，而是建立以下执行模型：

```text
GEMM Problem
-> 用 CTA Tile 覆盖 [M,N]
-> 沿 K 维分 Stage
-> Global Memory 到 Shared Memory
-> Shared Memory 到 Register Fragment
-> Warp/Warpgroup 发出 MMA
-> Accumulator 进入 Epilogue
-> 合并写回 Global Memory
```

能沿这条路径解释每个模板参数，才算真正读懂了 CUTLASS。 进一步阅读真实推理框架中的专用实现：[LMDeploy Grouped GEMM 源码详解：从 C++ 调度到 CUTLASS WGMMA](https://hd9568.github.io/blog/cuda-lmdeploy-cutlass-grouped-gemm-source/)。

- [CUTLASS GitHub](https://github.com/NVIDIA/cutlass)

- [CUTLASS C++ Quick Start](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/quickstart.html)

- [CUTLASS GEMM API](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/gemm_api.html)

- [Efficient GEMM in CUDA](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/efficient_gemm.html)

- [CUTLASS 3.x Design](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/cutlass_3x_design.html)

### CuTe DSL 入门：从 Layout、Tensor 到 GPU GEMM

> 原文：[CuTe DSL 入门：从 Layout、Tensor 到 GPU GEMM](https://hd9568.github.io/blog/cuda-cute-dsl-from-layout-to-gemm/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**面向掌握基础 CUDA 的读者，通过 Hello World、Vector Add、Layout 映射、Shared Memory GEMM 和 Tensor Core 骨架，讲清 CuTe DSL 的 JIT 编译、线程数据划分与执行过程。

#### 一、CuTe DSL 是什么

CuTe DSL 是 CUTLASS 4.x 中面向 GPU Kernel 的 Python DSL。它允许用 Python 语法描述：

- CUDA Grid、Block 和 Thread。

- Tensor 的 Shape、Stride 和地址空间。

- 数据怎样分给 CTA、Warp 和 Thread。

- Global/Shared/Register Memory 之间怎样 Copy。

- CUDA Core、Tensor Core 和异步 Pipeline 怎样执行。

它不是 NumPy，也不是“Python 在 GPU 上逐行解释”。

```text
Python 源码
-> AST Rewrite + Tracing
-> CuTe/MLIR 中间表示
-> NVIDIA GPU 相关 Lowering
-> PTX / CUBIN
-> CUDA Driver 启动 Kernel
```

最终运行在 GPU 上的是编译后的设备代码，Python 主要负责生成和启动它。

##### CUTLASS、CuTe C++、CuTe DSL 的关系

```text
CUTLASS:
  整个项目和高性能算子生态。
CuTe C++:
  CUTLASS 3.x 内部的 Layout/Tensor/Copy/MMA 模板库。
CuTe DSL:
  与 CuTe C++ 概念一致的 Python Kernel DSL。
```

CuTe DSL 比 CUTLASS C++ 少了大量模板语法，但它仍是低层编程模型。你依然需要理解 CUDA 线程、地址空间、Tiling、同步和 Tensor Core。

如果还不熟悉 CUTLASS 的 GEMM 分层，可以先阅读 [CUTLASS 入门：从 CUDA Tiling 到可组合 GEMM Kernel](https://hd9568.github.io/blog/cuda-cutlass-from-template-to-gemm/)。

##### 环境

官方 4.x 安装方式以 Linux 为主：

```bash
git clone --branch v4.7.1 https://github.com/NVIDIA/cutlass.git
# 根据对应 Tag 的 Quick Start 选择 CUDA 12/13。
./cutlass/python/CuTeDSL/setup.sh --cu12
pip install torch
```

安装后先确认：

```python
import cutlass
import cutlass.cute as cute
print(cutlass.__version__)
```

后文 Demo 需要 NVIDIA GPU 和兼容驱动。不同架构支持的 MMA、TMA 和 Pipeline 能力不同；Hello World 与普通 CUDA Core Demo 的硬件要求最低。

#### 二、Demo 1：Hello World 与两阶段执行

这是官方 `v4.7.1` 教学示例的核心代码：

```python
import cutlass
import cutlass.cute as cute
@cute.kernel
def hello_kernel(meta_arg: cutlass.Constexpr, dynamic_arg: cutlass.Int32):
    tidx, _, _ = cute.arch.thread_idx()
    if tidx == 1 or tidx == 3:
        cute.printf(
            "tidx={} meta_arg={} dynamic_arg={}",
            tidx,
            meta_arg,
            dynamic_arg,
        )
@cute.jit
def hello_host(meta_arg: cutlass.Constexpr, dynamic_arg: cutlass.Int32):
    hello_kernel(meta_arg, dynamic_arg).launch(
        grid=(1, 1, 1),
        block=(4, 1, 1),
    )
cutlass.cuda.initialize_cuda_context()
hello_host(10, 20)
```

预期输出：

```text
tidx=1 meta_arg=10 dynamic_arg=20
tidx=3 meta_arg=10 dynamic_arg=20
```

##### `@cute.kernel` 对应 `__global__`

CUDA C++：

```cpp
__global__ void kernel(int x) {
    int tid = threadIdx.x;
}
```

CuTe DSL：

```python
@cute.kernel
def kernel(x: cutlass.Int32):
    tid, _, _ = cute.arch.thread_idx()
```

两者都描述 GPU 设备代码。`cute.arch.thread_idx()` 最终会 Lower 到线程索引相关设备指令。

##### `@cute.jit` 是 Host 侧入口

Python 不能直接调用 `@cute.kernel`。调用链是：

```text
普通 Python
-> @cute.jit Host Function
-> kernel(...).launch(grid, block)
-> GPU Kernel
```

`@cute.jit` 负责构建 Launch 参数，也可以做编译期布局推导和 Kernel 组合。

##### `Constexpr` 与动态参数

```python
meta_arg: cutlass.Constexpr
dynamic_arg: cutlass.Int32
```

两者的区别：

```text
Constexpr:
  编译时已知。
  可用于 Tile 大小、循环展开和类型选择。
  值变化通常会产生另一个特化版本。
Int32:
  Kernel 运行时才知道。
  作为普通参数传入设备代码。
```

可以把 `Constexpr` 类比为 C++ 模板参数，把 `Int32` 类比为普通函数参数。

##### Python `print` 与 `cute.printf`

```python
print(x)
```

发生在 Python Meta-stage，用于查看编译器看到的 Shape、Layout 和代理值。

```python
cute.printf("x={}", x)
```

编译进 GPU 程序，在 Kernel 运行时输出。前者不能读取真实设备 Tensor 值，后者会显著拖慢 Kernel，只适合调试。

#### 三、Demo 2：从 CUDA Vector Add 理解 DSL Kernel

先回忆 CUDA C++：

```cpp
__global__ void add(const float* a, const float* b, float* c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}
```

对应的 CuTe DSL 教学写法：

```python
import torch
import cutlass
import cutlass.cute as cute
@cute.kernel
def add_kernel(
    gA: cute.Tensor,
    gB: cute.Tensor,
    gC: cute.Tensor,
    n: cutlass.Int32,
):
    tx, _, _ = cute.arch.thread_idx()
    bx, _, _ = cute.arch.block_idx()
    bdx, _, _ = cute.arch.block_dim()
    idx = bx * bdx + tx
    if idx < n:
        gC[idx] = gA[idx] + gB[idx]
@cute.jit
def add(a: cute.Tensor, b: cute.Tensor, c: cute.Tensor):
    threads = 256
    n = cute.size(c)
    blocks = cute.ceil_div(n, threads)
    add_kernel(a, b, c, n).launch(
        grid=(blocks, 1, 1),
        block=(threads, 1, 1),
    )
a = torch.randn(4096, device="cuda", dtype=torch.float32)
b = torch.randn_like(a)
c = torch.empty_like(a)
a_cute = cute.runtime.from_dlpack(a)
b_cute = cute.runtime.from_dlpack(b)
c_cute = cute.runtime.from_dlpack(c)
compiled_add = cute.compile(add, a_cute, b_cute, c_cute)
compiled_add(a_cute, b_cute, c_cute)
torch.testing.assert_close(c, a + b)
```

这段代码刻意使用一线程一元素，方便与基础 CUDA 对照。它不是最终高带宽版本。

##### Tensor 从哪里来

`from_dlpack` 不复制数据：

```text
torch.Tensor
  owns CUDA memory
cute.Tensor
  holds pointer + Layout
  views the same CUDA memory
```

因此 PyTorch 分配的 `c` 会被 CuTe Kernel 直接写入。

原始 PyTorch Tensor 必须保持存活；如果它被释放，CuTe Tensor 中的指针也会失效。

##### `cute.Tensor` 不只是 Pointer

普通 CUDA 参数是：

```cpp
float* ptr
```

CuTe Tensor 更接近：

```text
Tensor = Iterator/Pointer + Layout
```

当代码写：

```python
gA[idx]
```

CuTe 先用 Layout 把逻辑坐标 `idx` 映射到线性 Offset，再访问底层指针。

##### 为什么还需要 `cute.compile`

```python
compiled_add = cute.compile(add, a_cute, b_cute, c_cute)
```

第一次编译会根据参数类型、dtype、Layout 和 `Constexpr` 生成设备代码。之后：

```python
compiled_add(...)
```

只做参数封装和 Kernel Launch，不应把 JIT 编译时间算进 Kernel 延迟。

##### 怎样改成向量化

一线程一元素通常生成标量 Load/Store。若 FP32 每线程连续处理 4 个元素，并且地址 16-byte 对齐，编译器可以生成 128-bit 向量访存。

逻辑变化：

```text
thread 0 -> element 0..3
thread 1 -> element 4..7
...
```

CuTe 的正式写法不是随意强转 `float4*`，而是构造 Thread-Value Layout，显式描述：

```text
(thread_id, value_id) -> tile coordinate
```

这正是下一节的核心。

#### 四、Demo 3：Layout 是 CuTe 的核心

Layout 是一个函数：

```text
logical coordinate -> linear offset
```

它由 Shape 和 Stride 组成。

##### 最小二维例子

```python
@cute.jit
def show_layout():
    row_major = cute.make_layout(
        shape=(2, 4),
        stride=(4, 1),
    )
    col_major = cute.make_layout(
        shape=(2, 4),
        stride=(1, 2),
    )
    print("row_major =", row_major)
    print("col_major =", col_major)
    print("row_major(1,2) =", row_major(1, 2))
    print("col_major(1,2) =", col_major(1, 2))
show_layout()
```

Row-major 地址：

```text
offset(m, n) = m * 4 + n
       n=0 1 2 3
m=0 ->  0 1 2 3
m=1 ->  4 5 6 7
```

所以：

```text
row_major(1,2) = 1*4 + 2 = 6
```

Column-major：

```text
offset(m, n) = m + n * 2
       n=0 1 2 3
m=0 ->  0 2 4 6
m=1 ->  1 3 5 7
```

##### Shape 与 Stride 为什么都可以分层

CuTe 允许：

```text
Shape = ((2, 2), 4)
```

外层仍是二维，但第一维内部又分成两个 Mode。这能表达：

- 一个 Warp 中的 Lane 分组。

- 一条向量 Load 的多个值。

- MMA Fragment 中每个线程拥有的元素。

- Shared Memory Swizzle。

它不是为了把简单矩阵写复杂，而是为了用同一种代数表达 GPU 的层级结构。

##### `local_tile`：一个 CTA 拿哪块数据

假设：

```text
A shape = [M, K]
CTA tile = [BM, BK]
block coordinate = [block_m, block_k]
```

CuTe 可以写：

```python
gA = cute.local_tile(
    A,
    tiler=(BM, BK),
    coord=(block_m, block_k),
)
```

`gA` 仍然是 Tensor View，没有复制数据。它只是把原 Tensor 与新 Layout 组合，使坐标 `(i,j)` 指向当前 CTA 的元素。

##### Thread-Value Layout：每个线程搬哪些元素

设一个 Tile 有 8 个元素，4 个线程，每线程搬 2 个：

```text
thread 0 -> value 0, 1 -> tile element 0, 1
thread 1 -> value 0, 1 -> tile element 2, 3
thread 2 -> value 0, 1 -> tile element 4, 5
thread 3 -> value 0, 1 -> tile element 6, 7
```

这可以看成：

```text
TV Layout:
  (thread, value) -> logical element
```

CuTe 的 `TiledCopy` 持有这个映射：

```python
tiled_copy = cute.make_tiled_copy_tv(
    copy_atom,
    thr_layout,
    val_layout,
)
thr_copy = tiled_copy.get_slice(thread_idx)
src_per_thread = thr_copy.partition_S(src_tile)
dst_per_thread = thr_copy.partition_D(dst_tile)
cute.copy(copy_atom, src_per_thread, dst_per_thread)
```

`partition_S` 和 `partition_D` 没有发起 Copy，它们只回答：

```text
当前线程在 Source/Destination Tensor 中负责哪部分。
```

真正的数据移动发生在 `cute.copy`。

#### 五、Demo 4：Shared Memory Tiled GEMM

下面使用 CUTLASS 4.7.1 附带的教学 Primitive API，完整表达基础 CUDA Shared-Memory GEMM。它故意不使用 Tensor Core。

```python
import torch
import cutlass
import cutlass.cute as cute
from cutlass.experimental import primitives as prims
@cute.kernel
def gemm_kernel(
    a: cutlass.Array,
    b: cutlass.Array,
    c: cutlass.Array,
    TS: cutlass.Constexpr[int],
):
    tx, ty, _ = cute.arch.thread_idx()
    bx, by, _ = cute.arch.block_idx()
    a_smem = cutlass.Array(
        cutlass.Float32,
        (TS, TS),
        space=cutlass.AddressSpace.smem,
    )
    b_smem = cutlass.Array(
        cutlass.Float32,
        (TS, TS),
        space=cutlass.AddressSpace.smem,
    )
    K = a.shape[1]
    acc = 0.0
    for bk in range(0, K, TS):
        a_smem[ty, tx] = a[bx * TS + ty, bk + tx]
        b_smem[ty, tx] = b[bk + ty, by * TS + tx]
        prims.barrier_cta_sync(0)
        for k in range(TS):
            acc += a_smem[ty, k] * b_smem[k, tx]
        prims.barrier_cta_sync(0)
    c[bx * TS + ty, by * TS + tx] = acc
@cute.jit
def gemm(a, b, c, TS: cutlass.Constexpr[int]):
    M = a.shape[0]
    N = b.shape[1]
    gemm_kernel(a, b, c, TS).launch(
        grid=(M // TS, N // TS, 1),
        block=(TS, TS, 1),
    )
M = N = K = 1024
a = torch.randn(M, K, device="cuda")
b = torch.randn(K, N, device="cuda")
c = torch.zeros(M, N, device="cuda")
gemm(
    cute.runtime.from_dlpack(a),
    cute.runtime.from_dlpack(b),
    cute.runtime.from_dlpack(c),
    TS=32,
)
torch.testing.assert_close(c, a @ b, atol=1e-3, rtol=1e-3)
```

该 Demo 为了突出主线，要求 `M/N/K` 都能被 `TS` 整除。

##### 一个 Block 怎样计算

`TS=32` 时：

```text
block = (32, 32)
每个 Block 有 1024 个线程
每个线程计算 C Tile 中一个元素
```

对 `C[bx, by]` Tile：

```text
第 0 轮:
  加载 A[bx*32 : bx*32+32, 0:32]
  加载 B[0:32, by*32 : by*32+32]
第 1 轮:
  加载 K 维下一块
...
```

每轮两个 Barrier：

```text
第一个 Barrier:
  等所有线程完成 Shared Memory 写入。
第二个 Barrier:
  等所有线程用完当前 Tile，才能被下一轮覆盖。
```

##### 为什么它仍然不够快

这个版本的问题：

- 每个线程只算一个输出，寄存器复用较少。

- Global-to-Shared 是同步标量 Copy。

- 没有双缓冲，Load 与 Compute 串行。

- 使用 CUDA Core 标量 FMA，没有 Tensor Core。

- `32 x 32 = 1024` 线程让调度约束较强。

- 没有处理边界 Shape。

它的价值是把基础 CUDA 的执行模型一比一迁移到 DSL，而不是作为最终 GEMM。

#### 六、从教学 GEMM 到 Tensor Core GEMM

高性能 CuTe DSL GEMM 不再让“一个线程对应一个 C 元素”，而是先构造两个核心对象：

```text
TiledCopy:
  Copy Atom 在 Thread/Value Layout 上的重复。
  决定谁搬什么、一次搬多少、搬到哪里。
TiledMma:
  MMA Atom 在线程或 Warpgroup Layout 上的重复。
  决定谁持有 A/B/C Fragment、发出什么 MMA。
```

##### Tensor Core GEMM 的骨架

以下代码是教学骨架，不是可独立复制的完整 Kernel：

```python
@cute.kernel
def tensorcore_gemm(
    mA: cute.Tensor,
    mB: cute.Tensor,
    mC: cute.Tensor,
    tiled_copy_A: cute.TiledCopy,
    tiled_copy_B: cute.TiledCopy,
    tiled_mma: cute.TiledMma,
    sA_layout: cute.Layout,
    sB_layout: cute.Layout,
):
    tidx, _, _ = cute.arch.thread_idx()
    bidx, bidy, _ = cute.arch.block_idx()
    # 1. 当前 CTA 的 Global-Memory Tile。
    gA = cute.local_tile(mA, (128, 128, 32), (bidx, None, None))
    gB = cute.local_tile(mB, (128, 128, 32), (None, bidy, None))
    gC = cute.local_tile(mC, (128, 128, 32), (bidx, bidy, None))
    # 2. Shared Memory Tensor。
    smem = cutlass.utils.SmemAllocator()
    sA = smem.allocate_tensor(mA.element_type, sA_layout, 16)
    sB = smem.allocate_tensor(mB.element_type, sB_layout, 16)
    # 3. 当前线程的 Copy 分片。
    thr_copy_A = tiled_copy_A.get_slice(tidx)
    tAgA = thr_copy_A.partition_S(gA)
    tAsA = thr_copy_A.partition_D(sA)
    # 4. 当前线程/Warp 的 MMA 分片与寄存器 Accumulator。
    thr_mma = tiled_mma.get_slice(tidx)
    tCsA = thr_mma.partition_A(sA)
    tCsB = thr_mma.partition_B(sB)
    tCgC = thr_mma.partition_C(gC)
    tCrA = tiled_mma.make_fragment_A(tCsA)
    tCrB = tiled_mma.make_fragment_B(tCsB)
    accum = tiled_mma.make_fragment_C(tCgC)
    accum.fill(0.0)
    # 5. 沿 K Tile 执行 Mainloop。
    for k_tile in range(cute.size(gA, mode=[2])):
        cute.copy(tiled_copy_A, tAgA[None, None, k_tile], tAsA)
        cute.copy(tiled_copy_B, tBgB[None, None, k_tile], tBsB)
        cute.arch.sync_threads()
        cute.gemm(tiled_mma, accum, tCrA, tCrB, accum)
        cute.arch.sync_threads()
    # 6. 完整实现还需把 Accumulator 按输出 Copy Layout
    #    重排、转换类型并写回 tCgC；此处省略 Epilogue。
```

这里最重要的不是记函数名，而是看懂对象的变化：

```text
mA/mB:
  完整 Global-Memory Tensor
gA/gB:
  当前 CTA 的 Global-Memory Tile
sA/sB:
  当前 CTA 的 Shared-Memory Buffer
tAgA/tAsA:
  当前线程负责的 Global/Shared Copy Fragment
tCrA/tCrB:
  当前 MMA 参与者负责的 Operand Fragment
accum:
  Register 或新架构专用存储中的 Accumulator
```

##### Pipeline 怎样加入

教学版本是：

```text
Load tile 0 -> Compute tile 0
Load tile 1 -> Compute tile 1
```

多 Stage Pipeline 是：

```text
time --->
Load:    tile 0 | tile 1 | tile 2 | tile 3
Compute:          tile 0 | tile 1 | tile 2
```

Ampere 可用 `cp.async` 把 Global Memory 异步搬到 Shared Memory；Hopper 可用 TMA + WGMMA；Blackwell 可用 TMA + `tcgen05.mma` + TMEM。

CuTe DSL 中 Pipeline 对象管理：

```text
producer acquire:
  等待某 Stage 为空。
producer commit:
  标记 Load 已提交。
consumer wait:
  等待该 Stage 数据就绪。
consumer release:
  标记该 Stage 可再次覆盖。
```

这与手写 CUDA 双缓冲中的 Buffer Index、Barrier 和 Async Copy 是同一件事，只是被显式建模为可组合对象。

##### 为什么 Layout 决定性能

同一组数值可以有不同 Layout：

```text
Global Memory Layout:
  影响 Coalescing 和向量化。
Shared Memory Layout:
  影响 Bank Conflict 和 ldmatrix/TMA 约束。
Thread-Value Layout:
  影响每个 Lane 搬哪些元素。
MMA Fragment Layout:
  必须匹配硬件指令要求。
```

高性能 CuTe Kernel 的主要工作，就是让这些 Layout 在各级 Copy 和 MMA 之间“可组合且对齐”，而不是手写很多数组下标。

#### 七、编译缓存、调试与学习路线

##### Static 与 Dynamic Layout

显式转换：

```python
a_cute = cute.runtime.from_dlpack(a)
```

默认得到 Static Layout。Shape/Stride 会参与 JIT 特化，换 Shape 可能需要重新编译。

直接把 PyTorch Tensor 传给 `@cute.jit`，运行时会构造较动态的 Layout，多个 Shape 可以复用同一个编译结果，但编译器掌握的信息更少。

折中方式：

```python
a_cute = cute.runtime.from_dlpack(a)
a_cute = a_cute.mark_compact_shape_dynamic(
    mode=0,
    divisibility=128,
)
```

意思是第 0 维运行时可变，但保证能被 128 整除。编译器可以利用这个对齐信息。

##### 调试顺序

先查 Meta-stage：

```python
print(tensor.layout)
print(tile.shape)
print(thread_fragment.type)
```

再查 GPU Runtime：

```python
if tidx == 0 and bidx == 0:
    cute.printf("value={}", tensor[0])
```

最后使用：

```text
compute-sanitizer:
  越界、Race、未初始化访问。
Nsight Compute:
  Memory Coalescing、Bank Conflict、Tensor Core、
  Occupancy、Register 和 Stall。
生成 IR/PTX/SASS:
  确认是否生成向量 Load、cp.async、MMA/TMA。
```

##### 常见误区

```text
误区 1：Python 循环一定在 CPU 执行。
  @kernel 内的动态循环会编译成 GPU 循环；
  Constexpr 循环可能在编译期展开。
误区 2：Layout 就是 Row-major/Column-major。
  Layout 还能表达线程、Value、Tile、Fragment 和 Swizzle。
误区 3：partition_S 会复制数据。
  它只生成当前线程的 Tensor View；cute.copy 才搬数据。
误区 4：能用 cute.gemm 就自然高性能。
  TiledMma、Copy、Shared Layout、Pipeline 和 Shape 都必须匹配。
误区 5：JIT 时间就是 Kernel 时间。
  编译应独立缓存，性能测试只测编译后的调用。
```

##### 推荐学习顺序

```text
1. Hello World:
   分清 @jit、@kernel、Constexpr 和动态值。
2. Vector Add:
   对照 CUDA 的 Grid/Block/Thread 和边界判断。
3. Layout:
   手算 Shape/Stride 到 Offset 的映射。
4. TiledCopy:
   理解 (thread, value) 到 Tile Coordinate。
5. Shared-Memory GEMM:
   对照熟悉的 CUDA Tiling。
6. TiledMma:
   理解 MMA Atom、Fragment 和 Accumulator。
7. Pipeline:
   最后再学习 cp.async、TMA、WGMMA、TMEM。
```

CuTe DSL 最难的地方不是 Python 语法，而是它要求你把“线程怎样拥有数据”写成 Layout。掌握下面这条主线，就能开始阅读真实 Kernel：

```text
Tensor
-> local_tile 取 CTA Tile
-> TiledCopy.partition_* 分给线程
-> cute.copy 搬到 Shared/Register
-> TiledMma.partition_* 形成 MMA Fragment
-> cute.gemm 更新 Accumulator
-> Epilogue 写回 Global Memory
```

参考资料：

- [CuTe DSL Quick Start](https://docs.nvidia.com/cutlass/latest/media/docs/pythonDSL/quick_start.html)

- [CuTe DSL Programming Model](https://docs.nvidia.com/cutlass/latest/media/docs/pythonDSL/cute_dsl.html)

- [CuTe DSL Code Generation](https://docs.nvidia.com/cutlass/latest/media/docs/pythonDSL/cute_dsl_general/dsl_code_generation.html)

- [CuTe Layouts](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/cute/01_layout.html)

- [CuTe DSL Official Examples](https://github.com/NVIDIA/cutlass/tree/main/examples/python/CuTeDSL)

### LMDeploy Grouped GEMM 源码详解：从 C++ 调度到 CUTLASS WGMMA

> 原文：[LMDeploy Grouped GEMM 源码详解：从 C++ 调度到 CUTLASS WGMMA](https://hd9568.github.io/blog/cuda-lmdeploy-cutlass-grouped-gemm-source/)　作者：霍墨墨　来源：AI Infra Notes

> **内容提要：**沿 LMDeploy TurboMind 的 MoE FP8 Grouped GEMM 调用链，从 offsets、C++ 模板和运行时选核讲到 Persistent Scheduler、动态 TMA 描述符、CUTLASS Pipeline、Hopper WGMMA 与 Epilogue。

#### 一、先确定要读的 Kernel

LMDeploy 的 Grouped GEMM 不只有一种实现。本文选择下面这个模板实例：

```cpp
KernelImplSm90<
    GemmUniversalSm90_v5<
        kColMajor,  // Tile 的遍历顺序
        1,          // A multicast factor
        1,          // B multicast factor
        true        // grouped GEMM
    >
>
```

它在 Hopper `SM90` 上计算一个专用的 Block-scaled FP8 GEMM：

```text
A: FP8 E4M3，按 Expert 连续排列的激活
B: FP8 E4M3，每个 Expert 一个权重指针
U: FP32，A 沿 K 维分组的 Scale
V: FP32，B 的二维 Block Scale
D: BF16 输出
Accumulator: FP32
```

单个 Expert 的数学形式可以简化成：

```text
D_e = sum_k (A_e,k * U_e,k) @ (B_e,k * V_e,k)
```

实际 Kernel 不是先生成反量化后的 FP16 矩阵。它让 WGMMA 计算 FP8 乘法，再把对应的 `U/V` Scale 应用到 FP32 Partial Accumulator，避免额外反量化 Kernel 和中间 Buffer。

##### 它与 CUTLASS 是什么关系

这不是直接调用：

```cpp
cutlass::gemm::device::Gemm<...>
```

LMDeploy 自己实现了 Scheduler、Mainloop 和 Epilogue，但复用了 CUTLASS/CuTe 的底层原语：

```text
cute::SM90_TMA_STORE
cute::SM90_U32x4_STSM_N
cute::warpgroup_arrive / commit_batch / wait
cutlass::PipelineState
cutlass::arch::ClusterTransactionBarrier
cutlass::arch::ClusterBarrier
cutlass::arch::NamedBarrier
cutlass::arch::warpgroup_reg_alloc
```

所以更准确的描述是：

> LMDeploy 使用 CUTLASS/CuTe 的 Hopper 硬件抽象，拼装了一个面向 MoE FP8 的专用 Grouped GEMM。 如果还不熟悉 CTA Tile、Mainloop、Epilogue 和 C++ 模板，可先阅读 [CUTLASS 入门](https://hd9568.github.io/blog/cuda-cutlass-from-template-to-gemm/)。

##### 先记住调用链

```text
MoeFfnLayer::Forward
  生成 f2n / offsets
        |
        v
LlamaLinear::Forward
  重排并量化 Activation
  构造 MatrixLayout
        |
        v
gemm::Gemm::Run
  Filter -> Dispatch Cache / Heuristic
        |
        v
KernelImplSm90::Launch
  构造 Scheduler、TMA Descriptor、Persistent Grid
        |
        v
gemm_kernel_sm90<Kernel>
        |
        v
GemmUniversalSm90_v5::operator()
  Scheduler -> TMA Load -> WGMMA -> STSM -> TMA Store
```

这条路径涉及的核心源码如下：

| 文件 | 职责 |
| --- | --- |
| `models/llama/moe_ffn_layer.cc` | Router、Token 映射和 Expert offsets |
| `models/llama/LlamaLinear.cu` | 构造 Grouped GEMM 的 A/B/D 描述符 |
| `kernels/gemm/gemm.cu` | 过滤、选核、缓存与 Launch |
| `kernels/gemm/kernel_impl_sm90.h` | Host 侧 TMA、Grid、Cluster 配置 |
| `kernels/gemm/scheduler.cuh` | Persistent CTA 到 Expert Tile 的映射 |
| `kernels/gemm/gemm_universal_sm90_v5.h` | Producer/Consumer、WGMMA 和写回 |

#### 二、从普通 CUDA 理解 Grouped GEMM

##### Demo 1：最朴素的多个 Expert

设 Router 将 Token 分给三个 Expert：

```text
Expert 0: 3 tokens
Expert 1: 1 token
Expert 2: 4 tokens
offsets = [0, 3, 4, 8]
```

`offsets` 是 Prefix Sum。第 `e` 个 Expert 的行范围是：

```text
m0  = offsets[e]
m1  = offsets[e + 1]
M_e = m1 - m0
```

将 Token 按 Expert 排列后：

```text
A_grouped:
row 0 1 2 | row 3 | row 4 5 6 7
 expert 0 | exp 1 | expert 2
```

每个 Expert 计算：

```text
D_e[M_e, N] = A_e[M_e, K] @ B_e[K, N]
```

最容易理解的 C++ 写法是循环启动三个 GEMM：

```cpp
for (int e = 0; e < expert_num; ++e) {
    int m0 = offsets[e];
    int m1 = offsets[e + 1];
    int M  = m1 - m0;
    if (M == 0) continue;
    gemm<<<grid_for(M, N), block>>>(
        A + m0 * K,
        weight_ptrs[e],
        D + m0 * N,
        M, N, K);
}
```

它在数学上正确，但性能有三个问题：

```text
1. 每个 Expert 一次 Kernel Launch。
2. 小 M Expert 只有少量 CTA，无法占满 GPU。
3. 某个 Expert 结束后，它空闲的 SM 不能自动帮助其他 Expert。
```

Grouped GEMM 的目标不是改变矩阵乘法，而是把所有 Expert 的 Tile 放进同一个任务池，让固定数量的 CTA 持续取任务。

##### Demo 2：最小 Persistent Work Queue

先不考虑 CUTLASS，可以写出等价的调度骨架：

```cpp
__device__ int next_tile;
__global__ void persistent_grouped_gemm(Problem* problems, int total_tiles) {
    while (true) {
        int tile_id = atomicAdd(&next_tile, 1);
        if (tile_id >= total_tiles) return;
        Tile tile = map_global_tile_to_expert(tile_id, problems);
        compute_one_gemm_tile(tile);
    }
}
```

这里的 **Persistent** 表示 Grid 大小不再等于总 Tile 数。只启动接近 GPU 可同时驻留上限的 CTA，每个 CTA 完成一个 Tile 后继续领取下一个：

```text
普通 Grid:
  一个逻辑 Tile 对应一个 CTA
Persistent Grid:
  一个物理 CTA 在生命周期内处理多个逻辑 Tile
```

LMDeploy 的 `TileScheduler::next()` 正是在做 `atomicAdd + tile mapping`，只是还要处理 Cluster、Swizzle、五级 Pipeline 和不同 Expert 的边界。

#### 三、MoE 如何把 offsets 传到 GEMM

##### Router 先生成四组元数据

`MoeFfnLayer::Forward` 调用 `invokeMoeGate_V2`，得到：

```text
f2n:
  grouped row -> original token
f2E:
  grouped row -> expert id
en2f:
  (expert, original token) -> grouped row
offsets:
  每个 expert 在 grouped rows 中的起止位置
```

以 Top-2 Router 为例，一个 Token 会在 `f2n` 中出现两次，因为它要送到两个 Expert。有效行数为：

```text
M_total = token_num * experts_per_token
```

随后第一层 Expert Linear 调用：

```cpp
linear_.Forward(
    input,
    expert_weight,
    indices,   // f2n
    offsets,   // [expert_num + 1]
    output);
```

`indices` 负责说明每个 Grouped Row 来自哪个原始 Token，`offsets` 负责说明每个 Expert 拥有哪些 Grouped Row。

##### 激活为什么先重排

在 `LlamaLinear::GetOperandA` 中，FP8 路径执行：

```cpp
if (indices && A.dtype() == kFloat8_e4m3) {
    Tensor A_e = allocate({M_total, K});
    invokeMoeDispatch(A_e, A, indices.data(), experts_per_token, stream);
    A = A_e;
    indices = {};
}
```

这是源码语义的精简版。重排前，每个 Expert 的 Token 分散在原始输入中，需要 Indexed Gather；重排后：

```text
A = [A_0 rows][A_1 rows]...[A_E-1 rows]
```

Kernel 只需：

```cpp
A_e = A + offsets[e] * lda;
```

连续访问比每个元素读取索引更适合 TMA 大块搬运。 FP8 路径还会同步重排 Activation Scale `U`。这保证 `A` 的第 `r` 行与 `U` 的第 `r` 行仍对应同一个 Token。

##### Expert 权重为什么是 Device Pointer Array

各 Expert 权重是独立分配的 Tensor，不保证物理连续。`MoeWeight::LinkLinearExperts` 收集：

```cpp
weights = {
    {expert0_weight_ptr, ld0},
    {expert1_weight_ptr, ld1},
    ...
};
```

`MakeBlockedPtrs` 再生成 GPU 上的：

```text
B_ptrs = [B_0*, B_1*, ..., B_E-1*]
V_ptrs = [V_0*, V_1*, ..., V_E-1*]
```

这里必须区分两层指针：

```text
B_ptrs 本身在 Device Memory；
B_ptrs[e] 指向第 e 个 Expert 的 Device Weight。
```

##### `MatrixLayout` 把运行时信息送入后端

LMDeploy 用一个普通 C++ 结构描述矩阵：

```cpp
struct MatrixLayout {
    DataType type;
    Order order;
    int rows, cols, ld;
    Pack pack;
    int num;
    int* offsets;
    int* idxs;
};
```

Grouped 模式的关键字段是：

```text
rows    = 所有 Expert 的有效行总数 M_total
cols    = K 或 N
num     = Expert 数
offsets = Device 上的 int[E+1]
```

`get_mode()` 看到 `offsets != nullptr` 后，将矩阵标记为 `Striding::kBlocked`。这里的 Blocked 不是 CUTLASS Layout 的 Tensor Core Swizzle，而是“矩阵由多个 Expert Block 组成”。 完成描述符后，调用：

```cpp
gemm_.Run(
    operation,
    1.f,
    A, desc_A,
    U, desc_U,
    B_ptrs, desc_B,
    V_ptrs, desc_V,
    0.f,
    D, desc_D,
    D, desc_D,
    workspace,
    stream);
```

到这里仍在 Host 侧。真正的 CUDA Kernel 尚未启动。

#### 四、C++ 模板、注册表与运行时选核

这一层同时使用“编译期多态”和“运行时多态”，是初学者最容易迷失的地方。

##### 先补齐四个常见 C++ 语法

源码中的：

```cpp
using GroupedGemm = GemmUniversalSm90_v5<kColMajor, 1, 1, true>;
```

只是给复杂类型取别名，没有创建对象。真正创建对象的是：

```cpp
auto kernel = std::make_unique<KernelImplSm90<GroupedGemm>>();
```

`unique_ptr` 表示注册表独占该对象，注册表析构时自动释放，避免手写 `delete`。 `KernelImplSm90` 继承统一基类：

```cpp
class KernelImplSm90 : public Kernel {
    int Launch(...) override;
};
```

注册表保存 `Kernel*`，运行时通过虚函数调用不同模板实例的 `Launch`。这叫运行时多态：调用方不需要知道完整模板类型。 LlamaLinear 中的：

```cpp
auto&& [A, desc_A, U, desc_U] = GetOperandA(...);
```

叫 Structured Binding，它只是把返回的 `std::tuple` 拆成四个变量。`auto&&` 在这里避免无意义复制，并保持返回对象的值类别。 `Gemm::Run` 中还构造了 Lambda：

```cpp
const auto launch = [=](LaunchSpec spec, cudaStream_t stream) {
    return spec.kernel->Launch(/* captured arguments */, stream);
};
```

`[=]` 表示按值捕获外部变量。选核和实测代码只需反复调用 `launch(spec)`，不必重复写长参数列表。

##### 模板先生成具体 Kernel 类型

注册代码包含：

```cpp
using GroupedKernel =
    KernelImplSm90<
        GemmUniversalSm90_v5<kColMajor, 1, 1, true>>;
registry.Add(std::make_unique<GroupedKernel>());
```

四个模板参数分别表示：

```text
kColMajor:
  Scheduler 的 Tile Raster Order，不是 A/B 的矩阵布局。
1, 1:
  A/B 的 Cluster Multicast Factor。
true:
  编译 Grouped GEMM 分支。
```

`true` 是编译期常量，因此：

```cpp
if constexpr (is_grouped_gemm) {
    // Grouped 专用代码
}
```

在 Grouped 实例中会保留；普通 GEMM 实例中整段代码不会进入最终设备程序。 同一个模板还实例化 `(2,1)` 和 `(1,2)` 两种 Cluster 版本。运行时选核器根据 Shared Memory、Shape 和估算成本决定使用哪一个。

##### 外层 `KernelImplSm90` 是适配器

内层：

```cpp
GemmUniversalSm90_v5<...>
```

定义设备算法和所有编译期常量。外层：

```cpp
KernelImplSm90<Gemm>
```

继承统一的 `Kernel` 接口，负责：

```text
把模板常量转换成 Kernel Descriptor
检查硬件与 Shared Memory 是否可行
创建 TMA Descriptor
计算 Persistent Grid
调用 cudaLaunchKernelEx
```

这相当于：

```text
编译期：
  为每组模板参数生成一个高性能专用 Kernel。
运行时：
  通过 Kernel 基类在多个已编译实例中选择。
```

它不是在运行时重新生成模板。

##### Descriptor 如何筛选 Grouped Kernel

`KernelImplSm90` 的构造函数把 Grouped 属性写入描述符：

```cpp
desc_.striding_a = Striding::kBlocked;
desc_.striding_b = Striding::kBlocked;
desc_.striding_c = Striding::kBlocked;
desc_.group_axis = 0;
desc_.cta_tile   = {128, 96, 128};
desc_.stages     = 5;
desc_.arch       = 900;
```

同时写入：

```text
A/B: FP8 E4M3
D:   BF16
A Quant: K-axis group size 128
B Quant: 2D block group size 128
```

`Gemm::Run` 的过程可缩写为：

```cpp
Context ctx;
ctx.Init(operation, A_desc, U_desc, B_desc, V_desc, C_desc, D_desc);
auto feasible = ctx.Filter(registry.kernels());
auto spec = dispatch_cache.Find(ctx.desc());
if (!spec) {
    spec = choose_by_estimated_io_and_mma_cost(feasible);
}
spec.kernel->Launch(...);
```

`Dispatch Cache` 的 Key 包含架构、dtype、Layout、量化格式、Epilogue 和 Shape。Warmup 可以实测候选 Kernel，之后正式推理直接复用结果。

##### Host 侧怎样启动 Persistent Cluster

`KernelImplSm90::Launch` 不是按所有逻辑 Tile 创建 Grid，而是先受 SM 数和可驻留 Cluster 数限制：

```cpp
grid = min(
    sm_count,
    max_active_clusters * cluster_size);
block = CTA_SIZE;
```

然后通过扩展 Launch API 设置 Cluster：

```cpp
cudaLaunchAttribute attr{};
attr.id = cudaLaunchAttributeClusterDimension;
attr.val.clusterDim.x = cluster_size;
cudaLaunchKernelEx(&config, kernel, ...);
```

这里 `CTA_SIZE` 为：

```text
5 warp groups * 128 threads = 640 threads
```

一个 CTA 内有 4 个 Math Warpgroup 和 1 个 Producer Warpgroup。

#### 五、Persistent Scheduler 如何分配 Expert Tile

Scheduler 最终要把一个全局任务编号映射为：

```text
(expert_id, offset_m, offset_n, m0, m1)
```

其中：

```text
m0 = offsets[expert_id]
m1 = offsets[expert_id + 1]
M_e = m1 - m0
```

##### Demo 3：先手算 Tile 数

假设：

```text
offsets = [0, 130, 150, 420]
N = 192
CTA Tile = [128, 96]
```

三个 Expert 的逻辑 Tile 数：

```text
Expert 0:
  ceil(130 / 128) * ceil(192 / 96) = 2 * 2 = 4
Expert 1:
  ceil( 20 / 128) * ceil(192 / 96) = 1 * 2 = 2
Expert 2:
  ceil(270 / 128) * ceil(192 / 96) = 3 * 2 = 6
total = 12 tiles
```

如果只启动 4 个 Persistent CTA，它们会重复领取：

```text
CTA 0: tile 0 -> tile 4 -> tile 8
CTA 1: tile 1 -> tile 5 -> tile 9
CTA 2: tile 2 -> tile 6 -> tile 10
CTA 3: tile 3 -> tile 7 -> tile 11
```

真实顺序还会受到 Cluster 和 Swizzle 影响。

##### 全局原子计数器只分配 Cluster ID

`TileScheduler::next()` 的核心是：

```cpp
if (lane_id == 0) {
    wait_until_scheduler_stage_is_free();
    cluster_idx = atomicAdd(next_cluster_id_, 1);
}
cluster_idx = __shfl_sync(FULL_MASK, cluster_idx, 0);
```

只让一个 Lane 执行 `atomicAdd`，再用 Warp Shuffle 广播，避免 32 个线程重复争抢计数器。 `next_cluster_id_` 来自 Workspace。每次 Launch 前 Host 执行：

```cpp
cudaMemsetAsync(workspace.flags, 0, sizeof(int), stream);
```

所以所有常驻 CTA 从任务 0 开始共同消费。

##### 一个 `cluster_idx` 怎样定位 Expert

不同 Expert 的 `M_e` 不同，因此不能简单：

```text
expert_id = cluster_idx / fixed_tiles_per_expert
```

Scheduler 根据 `offsets` 计算每个 Expert 的起始任务位置。源码公式为：

```cpp
return (swizzle_tile_x_.div(offsets_[e]) + e)
       * swizzle_unit_.x
       * padded_cluster_tiles_.y;
```

`FastDivmod::div` 完成预计算除数的整数除法。公式中的额外 `e` 和 Padding 为每个 Expert 保留独立的 Swizzle 区域，使一个 Expert 的边界 Tile 不会与下一个 Expert 混在一起。 `update_sync()` 用一个 Warp 并行检查一段 Expert：

```cpp
pred = cluster_idx < start(candidate_expert);
mask = __ballot_sync(FULL_MASK, pred);
```

`__ballot_sync` 把 32 个布尔值压成 32-bit Mask，再通过 `__clz` 找到边界，从而确定当前任务属于哪个 Expert。它相当于一次 Warp 级分段查找，比每个线程独立做完整二分查找更适合这里。 找到 Expert 后再读取：

```cpp
group_m0 = offsets[e];
group_m1 = offsets[e + 1];
group_m  = group_m1 - group_m0;
```

最后 `unswizzle()` 得到该 Expert 内的 `(tile_m, tile_n)`，写入 `Tile1`：

```cpp
struct Tile1 {
    int is_valid_cta;
    int is_valid_cluster;
    int offset_m;
    int offset_n;
    int alive;
    int group_idx;
    int m0;
    int m1;
};
```

##### 为什么还需要 `alive` 和两种 Valid

Cluster 中多个 CTA 必须按 Cluster 语义协同。有时当前 CTA 已越过矩阵边界，但同 Cluster 的另一个 CTA 仍有效：

```text
is_valid_cta = false
is_valid_cluster = true
```

无效 CTA 不能直接退出，否则其他 CTA 可能永远等不到 Barrier 到达。它仍需推进 Pipeline、释放 Barrier，只跳过真实计算。 `alive=false` 才表示全局任务队列已经结束，可以退出 Persistent Loop。

#### 六、TMA 与 WGMMA 如何组成五级流水

##### Kernel 的线程分工

模板固定：

```cpp
WARPGROUPS = 4;
CTA_SIZE   = 128 * (4 + 1);
Stages     = 5;
```

线程角色为：

```text
Warpgroup 4:
  Producer。
  一个 Warp 负责 TMA Load；
  另一个 Warp 负责 Scheduler。
Warpgroup 0..3:
  Consumer / Math。
  两个 Warpgroup 组成一组，共同消费一个 CTA Tile。
```

Producer 调用：

```cpp
cutlass::arch::warpgroup_reg_dealloc<32>();
```

Math Warpgroup 调用：

```cpp
cutlass::arch::warpgroup_reg_alloc<112>();
```

Hopper 支持 Warpgroup Register Reconfiguration。Producer 只做地址和搬运控制，少分配寄存器；释放出的寄存器预算交给持有大量 FP32 Accumulator 的 Math Warpgroup。

##### Shared Memory 中放什么

源码中的 `SharedStorage` 可简化为：

```cpp
struct SharedStorage {
    FP8 A[5][128][128];
    FP8 B[5][ 96][128];
    FP32 U[5][aligned_M_scales];
    FP32 V[5][2];
    BF16 C[128][96];
    uint64_t producer_bar[5];
    uint64_t consumer_bar[5];
    CUtensorMap tma_desc[7];
    Scheduler::Storage scheduler;
};
```

每个 Stage 保存一个 K Tile：

```text
A Tile: [128, 128]
B Tile: [ 96, 128]
```

`Stages=5` 表示最多让五个 K Tile 处于不同的搬运/计算阶段。它不是把 K 维固定成 5 段；K 更大时 Ring Buffer 会循环复用这些 Stage。

##### Grouped GEMM 为什么需要动态 TMA Descriptor

普通 GEMM 的 B 基地址固定，Host 创建一次 `CUtensorMap` 即可。Grouped GEMM 的 B 是：

```text
B_ptrs[e]
```

每拿到一个新 Expert，TMA 的 Global Address 和 M 维范围都可能改变。 Producer 从 Tile 中取出：

```cpp
int e  = tile->group_idx;
int m0 = tile->m0;
int m1 = tile->m1;
```

然后准备三个地址：

```cpp
A_addr = A_base + m0 * lda;
B_addr = B_ptrs[e];
U_addr = U_base + aligned(m0);
```

接着用 Hopper Tensormap Replace 指令修改模板描述符：

```text
tensormap.replace.tile.global_address
tensormap.replace.tile.global_dim
tensormap.cp_fenceproxy
```

修改后的 Descriptor 写入 Workspace，再用：

```cpp
cute::tma_descriptor_fence_acquire(...)
```

保证 TMA Engine 看到新地址。这也是该实现要求 CUDA 12.3 以上的原因。

##### Producer 的五级循环

源码的核心可以还原成：

```cpp
PipelineState<5> write_state{producer_start};
for (int k_tile = 0; k_tile < ceil_div(K, 128); ++k_tile) {
    int stage = write_state.index();
    wait_until_consumers_release(stage);
    expect_transaction_bytes(stage, A_bytes + B_bytes + scale_bytes);
    tma_load(A[k_tile], smem_A[stage], producer_bar[stage]);
    tma_load(B[k_tile], smem_B[stage], producer_bar[stage]);
    tma_load(U[k_tile], smem_U[stage], producer_bar[stage]);
    cp_async(V[k_tile], smem_V[stage]);
    arrive_for_V_copy(producer_bar[stage]);
    ++write_state;
}
```

TMA 完成目标字节数后，`ProducerBar` 才切换 Phase，Consumer 才能读取该 Stage。

##### Consumer 如何执行 WGMMA

Consumer 维护自己的 `pipe_state`：

```cpp
wait(producer_bar[stage]);
load_scale_UV_from_smem();
reset_smem_descriptor_to_stage(stage);
GMMA::apply(A_smem_desc, B_smem_desc, accumulator, U, V);
release(consumer_bar[stage]);
++pipe_state;
```

`GMMA::apply` 最终选择类似：

```cpp
cute::SM90::GMMA::
MMA_64x96x32_F32E4M3E4M3_SS_TN
```

指令 Shape 表示：

```text
M = 64
N = 96
K = 32
输入 A/B = FP8 E4M3
输出累加 = FP32
SS = A/B 都来自 Shared Memory Descriptor
TN = 指令看到的 A/B 转置模式
```

WGMMA 是 Warpgroup 级异步矩阵乘。128 个线程协同发出指令，Accumulator 分散在 Warpgroup 的寄存器中。调用顺序包含：

```text
warpgroup_arrive
wgmma(...)
warpgroup_commit_batch
warpgroup_wait<N>
```

`commit_batch` 提交一批异步 WGMMA，`wait<N>` 限制尚未完成的批次数。这样地址计算、下一批 WGMMA 与之前的 Tensor Core 计算可以重叠。

##### Scale 为什么在 WGMMA 之后应用

`ScaledGmmaFP8_TN` 先得到一个 K Block 的 FP32 `frag_C`，然后执行：

```text
accum_C += frag_C * scale_A(m, k_block) * scale_B(n_block, k_block)
```

不能等整个 K 循环结束后只乘一次 Scale，因为每个 K Block 的 Scale 不同：

```text
C = sum_kblock [
      (A_fp8_block @ B_fp8_block)
      * U_scale(kblock)
      * V_scale(kblock)
    ]
```

这解释了为什么源码同时存在临时 `FragC` 和最终 `AccumC`：前者接收本次 WGMMA Batch，后者累加已经应用 Scale 的结果。

#### 七、Epilogue、完整执行链与阅读方法

##### 从寄存器写回 BF16

K 循环完成后，结果仍分散在每个 Lane 的 FP32 Accumulator 中。源码通过 `GMMA::foreach_C` 遍历线程拥有的 Fragment：

```cpp
GMMA::foreach_C(accum_C, [&](auto const& frag, int m, int n) {
    Array<bf16, 8> out = cast<bf16>(frag);
    cute::SM90_U32x4_STSM_N::copy(
        out[0], out[1], out[2], out[3],
        smem_C[swizzled_offset(m, n)]);
});
```

这里发生三件事：

```text
FP32 -> BF16 类型转换
寄存器 Fragment -> Shared Memory 矩阵重排
Shared Memory Swizzle 以满足写回访问模式
```

`STSM` 是寄存器到 Shared Memory 的协作存储。Math Warpgroup 完成后用 `NamedBarrier` 同步，再由部分线程执行 TMA Store：

```cpp
cute::SM90_TMA_STORE::copy(
    output_tma_desc,
    smem_C,
    global_n,
    global_m);
```

Grouped 模式下，输出 Descriptor 也会按：

```text
D_base + offsets[e] * ldd
```

动态更新，因此结果仍按 Expert 连续排列。后续 `invokeMoeCombine` 再通过 `en2f/f2E` 把 Expert 输出乘 Router Weight，并 Scatter-Reduce 回原 Token 顺序。

##### 一次请求的完整执行顺序

把所有细节压缩成一条链：

```text
1. Router:
   为每个 Token 选择 Top-K Expert。
2. Prefix Sum:
   生成 offsets[e]，确定每个 Expert 的 M_e。
3. Dispatch:
   按 Expert 重排 Token，得到连续 A_grouped。
4. Descriptor:
   A/D 用 offsets 切分；
   B/V 用 Device Pointer Array 选择 Expert。
5. Dispatch:
   Registry 过滤 dtype/layout/quant/arch；
   Cache 或估价器选择具体模板实例。
6. Persistent Launch:
   固定数量 CTA/Cluster 共同消费全局 Tile Queue。
7. Dynamic TMA:
   每个 Tile 根据 expert_id 修改 A/B/U/D 描述符。
8. Mainloop:
   五级 TMA Pipeline 搬 A/B/U/V；
   WGMMA 计算 FP8 Tile并应用 Scale。
9. Epilogue:
   FP32 Accumulator 转 BF16，经 STSM 重排后 TMA Store。
10. Combine:
    按 Router Weight 聚合回原 Token。
```

##### 性能为什么可能更好

主要收益来自：

```text
一次 Launch 覆盖所有 Expert
Persistent CTA 跨 Expert 动态负载均衡
TMA 隐藏 Global Memory 搬运
WGMMA 提供 FP8 Tensor Core 吞吐
Warp Specialization 让调度/搬运与计算并行
寄存器重分配给 Accumulator
STSM + TMA Store 降低 Epilogue 指令开销
```

代价也很明确：

```text
动态 TMA Descriptor 修改有固定开销
小 Expert 的边界 Tile 利用率低
Global Atomic Scheduler 可能竞争
五级 Shared Memory 占用限制驻留 CTA
Cluster 内无效 CTA 仍需参与同步
```

所以 Grouped Kernel 不一定在所有 Shape 上获胜。LMDeploy 保留 Registry、Heuristic、实测 Tuning 和 Dispatch Cache，就是为了避免把一个模板用于所有场景。

##### 阅读这类 Kernel 的顺序

不要从 `GMMA::apply` 内层开始。更有效的顺序是：

```text
先看 Tensor Shape:
  A/B/U/V/D 分别是什么。
再看 Tile:
  CTA_M/N/K、Stage、Warpgroup 数。
再看 Ownership:
  offsets 和 pointer array 如何定位 Expert。
再看 Scheduler:
  一个物理 CTA 怎样领取多个逻辑 Tile。
再看 Pipeline:
  谁生产、谁消费、何时等待和释放。
最后看 Instruction:
  WGMMA Shape、Fragment Layout、STSM 和 TMA Store。
```

如果能回答下面五个问题，就已经读通了这段代码：

```text
1. 当前 Tile 属于哪个 Expert？
2. A/B/U/V 的真实地址怎样得到？
3. 当前 Shared Memory Stage 何时可以覆盖？
4. 哪 128 个线程共同拥有一次 WGMMA 的 Fragment？
5. 寄存器中的结果怎样回到正确 Expert 的 D 区间？
```

这也是从基础 CUDA 过渡到 CUTLASS/CuTe 源码时最重要的方法：先沿数据和所有权走一遍，再理解模板与指令。 参考源码：

- [MoE Router 与 Grouped Linear 调用](https://github.com/InternLM/lmdeploy/blob/b5cf542e2c9f51898e5195ecbbc25f0836de1c4e/src/turbomind/models/llama/moe_ffn_layer.cc)

- [LlamaLinear 描述符构造](https://github.com/InternLM/lmdeploy/blob/b5cf542e2c9f51898e5195ecbbc25f0836de1c4e/src/turbomind/models/llama/LlamaLinear.cu)

- [SM90 Grouped Kernel 注册](https://github.com/InternLM/lmdeploy/blob/b5cf542e2c9f51898e5195ecbbc25f0836de1c4e/src/turbomind/kernels/gemm/kernel/sm90_64n32_8.cu)

- [Persistent Tile Scheduler](https://github.com/InternLM/lmdeploy/blob/b5cf542e2c9f51898e5195ecbbc25f0836de1c4e/src/turbomind/kernels/gemm/scheduler.cuh)

- [GemmUniversalSm90 v5](https://github.com/InternLM/lmdeploy/blob/b5cf542e2c9f51898e5195ecbbc25f0836de1c4e/src/turbomind/kernels/gemm/gemm_universal_sm90_v5.h)

- [CUTLASS Hopper Pipeline](https://docs.nvidia.com/cutlass/latest/media/docs/cpp/pipeline.html)
