#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <regex.h>

char * map_file(const char * file, int * psize)
{
    int fd;
    fd = open(file, O_RDWR);
    if(fd < 0)
    {
        perror("open");
        exit(1);
    }
    int size;
    size = lseek(fd, 0, SEEK_END);
    *psize = size;
    char * ptr = NULL;
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(ptr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        exit(1);
    }
    close(fd);
    return ptr;
}

int main(void)
{
    //映射文件
    char * ptr = NULL;
    int psize;
    ptr = map_file("测试.txt", &psize);
    char * copy_ptr = NULL;
    copy_ptr = ptr; //备份首地址，用于munmap

    regex_t preg;
    /*
     * 正则：<a 任意属性 href=" " 任意属性>(捕获文本)</a >
     * 注意：C字符串里 \ 要写成 \\
     * 分组1：href链接；分组2：a标签内标题文本
     */
    char * regex_str = "<a[ \t]+.*href=\"([^\"]+)\".*>([^<]+)</a >";
    size_t nmatch = 3;   // 0:整个匹配, 1:href, 2:标题
    regmatch_t pmatch[nmatch];
    char href_buf[1024];
    char title_buf[1024];
    int err;

    // 编译正则，REG_EXTENDED 使用扩展正则
    if((err = regcomp(&preg, regex_str, REG_EXTENDED)) != 0)
    {
        char errbuf[256];
        regerror(err, &preg, errbuf, sizeof(errbuf));
        printf("regcomp error: %s\n", errbuf);
        return -1;
    }

    //循环查找
    while((regexec(&preg, ptr, nmatch, pmatch, 0)) == 0)
    {
        bzero(href_buf, sizeof(href_buf));
        bzero(title_buf, sizeof(title_buf));

        //提取href 分组1
        int href_len = pmatch[1].rm_eo - pmatch[1].rm_so;
        snprintf(href_buf, href_len + 1, "%s", ptr + pmatch[1].rm_so);

        //提取标题 分组2
        int title_len = pmatch[2].rm_eo - pmatch[2].rm_so;
        snprintf(title_buf, title_len + 1, "%s", ptr + pmatch[2].rm_so);

        printf("链接：%s\n标题：%s\n----------------\n", href_buf, title_buf);

        //指针向后偏移，防止死循环重复匹配
        ptr += pmatch[0].rm_eo;
    }

    regfree(&preg);       //释放正则
    munmap(copy_ptr, psize); //释放内存映射
    return 0;
}