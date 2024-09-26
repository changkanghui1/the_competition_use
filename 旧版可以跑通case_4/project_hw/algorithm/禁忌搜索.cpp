
/*
主要特点
邻域搜索：

禁忌搜索从当前解出发，通过对解进行小的修改（如交换、插入等）生成邻域解。
在每一轮迭代中，算法会探索这些邻域解。
禁忌表：

为了防止算法在解空间中循环，禁忌搜索使用一个禁忌表来记录最近访问过的解或操作。
如果某个解或操作在禁忌表中，则不允许再次选择，通常会设定一个禁忌期，过后才可以再次选择。
选择准则：

在选择邻域解时，禁忌搜索会优先考虑具有最佳目标函数值的解，即使它们在禁忌表中。
也可能允许选择一些禁忌解，但需要设定某种惩罚机制，以降低它们的吸引力。
动态更新：

禁忌表会动态更新，随着搜索的进行，某些解会从禁忌表中移除，从而重新允许访问。
多样性控制：

禁忌搜索通过限制某些解的选择，增加了解的多样性，有助于探索更广泛的解空间。
*/

// 禁忌搜索的主要函数
void TabuSearch(const InputParam *input, OutputParam *output) {
    uint32_t *currentSequence = (uint32_t *)malloc(output->len * sizeof(uint32_t));
    uint32_t *bestSequence = (uint32_t *)malloc(output->len * sizeof(uint32_t));
    
    // 使用贪心算法生成的初始顺序
    for (uint32_t i = 0; i < output->len; i++) {
        currentSequence[i] = output->sequence[i];
    }
    
    memcpy(bestSequence, currentSequence, output->len * sizeof(uint32_t));
    double bestCost = CalculateTotalCost(input, bestSequence);
    
    int *tabuList = (int *)calloc(MAX_TABS, sizeof(int));
    int tabuIndex = 0;
    
    for (int iter = 0; iter < MAX_ITER; iter++) {
        uint32_t bestNeighborSequence[output->len];
        double bestNeighborCost = INT_MAX;

        // 生成邻居解
        for (uint32_t i = 0; i < output->len - 1; i++) {
            for (uint32_t j = i + 1; j < output->len; j++) {
                // 交换 i 和 j
                uint32_t tempSeq[output->len];
                memcpy(tempSeq, currentSequence, output->len * sizeof(uint32_t));
                uint32_t temp = tempSeq[i];
                tempSeq[i] = tempSeq[j];
                tempSeq[j] = temp;

                // 计算邻居解的成本
                double newCost = CalculateTotalCost(input, tempSeq);
                
                // 检查是否是最优邻居
                if (newCost < bestNeighborCost && !tabuList[tempSeq[i] % MAX_TABS] && !tabuList[tempSeq[j] % MAX_TABS]) {
                    bestNeighborCost = newCost;
                    memcpy(bestNeighborSequence, tempSeq, output->len * sizeof(uint32_t));
                }
            }
        }

        // 更新当前解
        if (bestNeighborCost < bestCost) {
            bestCost = bestNeighborCost;
            memcpy(bestSequence, bestNeighborSequence, output->len * sizeof(uint32_t));
            // 将当前解添加到禁忌表
            tabuList[currentSequence[0] % MAX_TABS] = 1;  // 可以选择某种策略填充禁忌表
        }

        memcpy(currentSequence, bestNeighborSequence, output->len * sizeof(uint32_t));

        // 更新禁忌表
        if (tabuIndex < MAX_TABS) {
            tabuIndex++;
        } else {
            memset(tabuList, 0, MAX_TABS * sizeof(int));  // 清空禁忌表
        }
    }

    memcpy(output->sequence, bestSequence, output->len * sizeof(uint32_t));
    
    free(currentSequence);
    free(bestSequence);
    free(tabuList);
}
/*
优化说明
    禁忌搜索：这种算法在生成邻居解时引入了禁忌表，能够避免在禁忌期内再次选择那些解。这增加了搜索的多样性，能够更好地探索解空间。

    邻居生成：通过交换两个位置来生成邻居解，适用于 I/O 调度的顺序优化。

    禁忌表：使用禁忌表来记录最近的解，避免局部最优陷阱。

    内存管理：确保在最后释放所有分配的内存，防止内存泄漏。

*/