#include "algorithm.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>  // 包含用于 memcpy 的头文件

#define MAX_ITER 10000  // 最大迭代次数
#define INIT_TEMP 10000.0  // 初始温度
#define COOLING_RATE 0.995  // 冷却速率

// 寻址时间计算
// uint32_t SeekTimeCalculate(const HeadInfo *start, const HeadInfo *target) {
//     int32_t wrap_diff = abs((int32_t)(start->wrap - target->wrap)); // wrap 表示磁道号，跨一条磁带分配用时
//     int32_t lpos_diff = abs((int32_t)(start->lpos - target->lpos)); // lpos 表示磁头位置，每移动一个相对位置分配用时
//     return wrap_diff * 1000 + lpos_diff;  // 假设每wrap的转换耗时1000ms，每lpos耗时1ms
// }

// 计算调度序列的总寻址时间
double CalculateTotalCost(const InputParam *input, const uint32_t *sequence) {
    double totalCost = 0;
    HeadInfo currentHead = input->headInfo;

    for (uint32_t i = 0; i < input->ioVec.len; i++) { // 遍历所有IO请求
        IOUint *io = &input->ioVec.ioArray[sequence[i]]; // 获取当前IO请求
        HeadInfo target = {io->wrap, io->startLpos, HEAD_RW};

        // 只计算寻址时间
        uint32_t seekTime = SeekTimeCalculate(&currentHead, &target);
        totalCost += seekTime;

        // 更新磁头位置
        currentHead.wrap = io->wrap;
        currentHead.lpos = io->endLpos;
    }

    return totalCost;
}

// 贪心算法：选择最近的IO请求
void GreedySchedule(const InputParam *input, OutputParam *output) {
    uint32_t remaining = input->ioVec.len;
    int *visited = (int *)calloc(remaining, sizeof(int)); // 记录每个IO请求是否已被访问，打开一个数组
    
    HeadInfo currentHead = input->headInfo;

    for (uint32_t i = 0; i < output->len; i++) {
        int nearestIndex = -1; //定义一个最小值，找到最近的IO请求来更新
        uint32_t nearestTime = UINT32_MAX;

        // 找到最近的IO请求
        for (uint32_t j = 0; j < input->ioVec.len; j++) { //j表示第几个IO请求
            if (visited[j]) continue;

            IOUint *io = &input->ioVec.ioArray[j];
            HeadInfo target = {io->wrap, io->startLpos, HEAD_RW}; // 获取当前IO请求的磁头位置和磁带编号，便于计算寻址时间
            uint32_t seekTime = SeekTimeCalculate(&currentHead, &target);

            if (seekTime < nearestTime) { // 找到更近的IO请求，更新下标
                nearestTime = seekTime;
                nearestIndex = j;
            }
        }

        if (nearestIndex != -1) { //出现过更近的IO请求，更新是否被访问
            visited[nearestIndex] = 1;
            output->sequence[i] = input->ioVec.ioArray[nearestIndex].id; // 将最近的IO请求的id添加到输出序列中

            // 更新磁头位置，从已经找到的IO请求中开始作为贪心算法的起点
            currentHead.wrap = input->ioVec.ioArray[nearestIndex].wrap;
            currentHead.lpos = input->ioVec.ioArray[nearestIndex].endLpos;
        }
    }

    free(visited);
}

// 模拟退火算法：进一步优化贪心算法生成的初始调度序列
void SimulatedAnnealing(const InputParam *input, OutputParam *output) {
    uint32_t *currentSequence = (uint32_t *)malloc(output->len * sizeof(uint32_t));
    uint32_t *bestSequence = (uint32_t *)malloc(output->len * sizeof(uint32_t));

    // 使用贪心算法生成的初始顺序
    for (uint32_t i = 0; i < output->len; i++) {
        currentSequence[i] = output->sequence[i];
    }

    // 记录当前最佳序列
    memcpy(bestSequence, currentSequence, output->len * sizeof(uint32_t));
    double bestCost = CalculateTotalCost(input, bestSequence);

    // 初始温度和冷却速率
    double temp = INIT_TEMP;

    for (int iter = 0; iter < MAX_ITER; iter++) {
        // 生成新的序列（随机交换两个位置）
        uint32_t newSequence[output->len];
        memcpy(newSequence, currentSequence, output->len * sizeof(uint32_t));

        uint32_t idx1 = rand() % output->len;
        uint32_t idx2 = rand() % output->len;
        uint32_t tempVal = newSequence[idx1];
        newSequence[idx1] = newSequence[idx2];
        newSequence[idx2] = tempVal;

        // 计算新序列的寻址时间
        double newCost = CalculateTotalCost(input, newSequence);

        // 计算接受新序列的概率
        double acceptanceProbability = exp((bestCost - newCost) / temp);
        if (newCost < bestCost || ((double)rand() / RAND_MAX) < acceptanceProbability) {
            memcpy(currentSequence, newSequence, output->len * sizeof(uint32_t));
            if (newCost < bestCost) {
                bestCost = newCost;
                memcpy(bestSequence, newSequence, output->len * sizeof(uint32_t));
            }
        }

        // 降温
        temp *= COOLING_RATE;
    }

    // 记录最终最优解
    memcpy(output->sequence, bestSequence, output->len * sizeof(uint32_t));

    free(currentSequence);
    free(bestSequence);
}

// 主算法接口：结合贪心和模拟退火的混合算法
int32_t IOScheduleAlgorithm(const InputParam *input, OutputParam *output) {
    if (input == NULL || output == NULL) return RETURN_ERROR;

    output->len = input->ioVec.len;

    // 先使用贪心算法生成初始调度顺序
    GreedySchedule(input, output);

    // 使用模拟退火算法在贪心基础上进一步优化
    SimulatedAnnealing(input, output);

    return RETURN_OK;
}

void PrintMetrics(const KeyMetrics *metrics)
{
    printf("\nKey Metrics:\n");
    printf("\talgorithmRunningDuration:\t %f ms\n", metrics->algorithmRunningDuration);
    printf("\taddressingDuration:\t\t %u ms\n", metrics->addressingDuration);
    printf("\treadDuration:\t\t\t %u ms\n", metrics->readDuration);
    printf("\ttapeBeltWear:\t\t\t %u\n", metrics->tapeBeltWear);
    printf("\ttapeMotorWear:\t\t\t %u\n", metrics->tapeMotorWear);
}

/**
 * @brief  算法运行的主入口
 * @param  input            输入参数
 * @param  output           输出参数
 * @return uint32_t          返回成功或者失败，RETURN_OK 或 RETURN_ERROR
 */
uint32_t AlgorithmRun(const InputParam *input, OutputParam *output)
{
    int32_t ret;
    KeyMetrics metrics = {0};

    struct timeval start, end;
    long seconds, useconds;
    // 记录开始时间
    gettimeofday(&start, NULL);

    ret = IOScheduleAlgorithm(input, output);

    // 记录结束时间
    gettimeofday(&end, NULL);

    // 计算秒数和微秒数
    seconds = end.tv_sec - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;

    // 总微秒数
    metrics.algorithmRunningDuration = ((seconds)*1000000 + useconds);

    /* 访问时间 */
    TotalAccessTime(input, output, &metrics.addressingDuration, &metrics.readDuration);

    /* 带体磨损 */
    metrics.tapeBeltWear = TotalTapeBeltWearTimes(input, output, metrics.lposPassTime);

    /* 电机磨损 */
    metrics.tapeMotorWear = TotalMotorWearTimes(input, output);

    PrintMetrics(&metrics);

    return RETURN_OK;
}
