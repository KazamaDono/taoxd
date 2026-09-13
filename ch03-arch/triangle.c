/* triangle.c - sum 1..n in a loop.  Small enough that the disassembly
 * fits on a page, which is the point: read the two ISAs side by side. */
long triangle(long n)
{
    long sum = 0;
    for (long i = 1; i <= n; i++)
        sum += i;
    return sum;
}

int main(void)
{
    return (int) triangle(100);   /* 5050, truncated into the exit status */
}
