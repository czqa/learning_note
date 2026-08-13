#include <stdio.h>
// 合并两个有序子序列
// 第一个子序列是 data[left...mid]
// 第二个子序列是 data[mid+1...right]
void merge(int* data, int left, int mid, int right)
{
    int temp[100]; // 临时数组，用于存放合并结果
    int i = left;      // 左半部分起始索引
    int j = mid + 1;   // 右半部分起始索引
    int k = 0;         // temp 数组下标
    // 将两个有序部分按照大小关系依次放入 temp 中
    while (i <= mid && j <= right)
    {
        if (data[i] <= data[j])
        {
            temp[k++] = data[i++];
        }
        else
        {
            temp[k++] = data[j++];
        }
    }
    // 将剩余的左边部分拷贝到 temp 中（如果有）
    while (i <= mid)
    {
        temp[k++] = data[i++];
    }
    // 将剩余的右边部分拷贝到 temp 中（如果有）
    while (j <= right)
    {
        temp[k++] = data[j++];
    }
    // 将 temp 中的结果拷贝回原数组中，完成合并
    for (int t = 0; t < k; t++)
    {
        data[left + t] = temp[t];
    }
}
// 归并排序的递归函数
// 对 data[left...right] 这一段进行排序
void mergeSort(int* data, int left, int right)
{
    if (left < right) // 至少两个元素才需要排序
    {
        int mid = (left + right) / 2;

        // 对左半部分排序
        mergeSort(data, left, mid);

        // 对右半部分排序
        mergeSort(data, mid + 1, right);

        // 将排好序的左右两部分合并
        merge(data, left, mid, right);
    }
}

int main(int argc, char const *argv[])
{
    int data[] = {47, 35, 60, 95, 77, 15, 28, 52, 80};
    int len = sizeof(data) / sizeof(data[0]);
    mergeSort(data, 0, len - 1); // 对整个数组进行排序
    // 输出排序后的结果
    for (int i = 0; i < len; i++)
    {
        printf("%d ", data[i]);
    }
    return 0;
}