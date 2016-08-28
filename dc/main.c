#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <error.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "libconfig.h"
#include "shm.h"

config_t cfg;

char *usage =  "[功能：参数设置查看] \n"
               "\tdc analog\t遥测参数配置\n"
               "\tdc domain\t遥控参数配置\n"
               "\tdc remote\t遥信参数配置\n"
               "\tdc protect\t保护参数配置\n"
               "\tdc an 0\t查看遥测数据 0 代表第一块板,以此类推.\n"
               "\tdc re\t查看最新的 25 条 soe 记录.\n"
               "[功能：校表]\n"
               "校表功能暂未开放";

int dealAnalog(int argc, char *argv[]) {
    char *usage = "使用方法:\n"
                  "\tdc analog show\n"
                  "\tdc analog add\n"
                  "\tdc analog delete\n"
                  "\tdc analog set\n";
    if (argc == 2) {
        printf("%s\n", usage);
        return 1;
    }

    if (!strcmp("show", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.analog");
        int length = config_setting_length(setting);

        printf("号\t开关\t板\t路\t超时\t控\t分合\t归一\t放大\t阀值\t下限\t上限\n");

        for (int i = 0; i < length; ++i) {
            int n, s, b, l, otime, c, w;
            double f, la, threshold, down, up;
            config_setting_t *r = config_setting_get_elem(setting, i);
            config_setting_lookup_int(r, "n", &n);
            config_setting_lookup_int(r, "s", &s);
            config_setting_lookup_int(r, "b", &b);
            config_setting_lookup_int(r, "l", &l);
            config_setting_lookup_int(r, "time", &otime);
            config_setting_lookup_int(r, "c", &c);
            config_setting_lookup_int(r, "w", &w);

            config_setting_lookup_float(r, "f", &f);
            config_setting_lookup_float(r, "la", &la);
            config_setting_lookup_float(r, "th", &threshold);
            config_setting_lookup_float(r, "down", &down);
            config_setting_lookup_float(r, "up", &up);

            printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%5.1f\t%5.1f\t%5.1f\t%5.1f\t%5.1f\n", n, s, b, l, otime, c, w, f, la, threshold, down, up);
        }
        return 0;
    }
    if (!strcmp("add", argv[2])) {
        if (argc == 3) {
            printf("号\t开关\t板\t路\t超时\t控\t分合\t归一\t放大\t阀值\t下限\t上限\n");
            return 0;
        }
        if (argc == 15) {
            config_setting_t *array = config_lookup(&cfg, "go.analog");
            config_setting_t * group, * r;

            group = config_setting_add(array, NULL, CONFIG_TYPE_GROUP);

            r = config_setting_add(group, "n", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[3]));
            r = config_setting_add(group, "s", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[4]));
            r = config_setting_add(group, "b", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[5]));
            r = config_setting_add(group, "l", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[6]));
            r = config_setting_add(group, "otime", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[7]));
            r = config_setting_add(group, "c", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[8]));
            r = config_setting_add(group, "w", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[9]));

            r = config_setting_add(group, "f", CONFIG_TYPE_FLOAT);
            config_setting_set_float(r, atof(argv[10]));
            r = config_setting_add(group, "la", CONFIG_TYPE_FLOAT);
            config_setting_set_float(r, atof(argv[11]));
            r = config_setting_add(group, "th", CONFIG_TYPE_FLOAT);
            config_setting_set_float(r, atof(argv[12]));
            r = config_setting_add(group, "down", CONFIG_TYPE_FLOAT);
            config_setting_set_float(r, atof(argv[13]));
            r = config_setting_add(group, "up", CONFIG_TYPE_FLOAT);
            config_setting_set_float(r, atof(argv[14]));

            config_write_file(&cfg, "config.cfg");
            return 0;
        }

        printf("参数数量错误\n");
        return 0;
    }
    if (!strcmp("delete", argv[2])) {
        if (argc == 4) {
            config_setting_t *setting = config_lookup(&cfg, "go.analog");
            int length = config_setting_length(setting);

            for (int i = 0; i < length; ++i) {
                int n = 0;
                config_setting_t *r = config_setting_get_elem(setting, i);
                config_setting_lookup_int(r, "n", &n);

                if (n == atoi(argv[3])) {
                    config_setting_remove_elem(setting, i);
                    config_write_file(&cfg, "config.cfg");
                }
            }
        }
        return 0;
    }
    if (!strcmp("set", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.analog");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);
            config_setting_lookup_int(elem, "n", &n);

            if (n == atoi(argv[3])) {
                r = config_setting_lookup(elem, "n");
                config_setting_set_int(r, atoi(argv[3]));
                r = config_setting_lookup(elem, "s");
                config_setting_set_int(r, atoi(argv[4]));
                r = config_setting_lookup(elem, "b");
                config_setting_set_int(r, atoi(argv[5]));
                r = config_setting_lookup(elem, "l");
                config_setting_set_int(r, atoi(argv[6]));
                r = config_setting_lookup(elem, "time");
                config_setting_set_int(r, atoi(argv[7]));
                r = config_setting_lookup(elem, "c");
                config_setting_set_int(r, atoi(argv[8]));
                r = config_setting_lookup(elem, "w");
                config_setting_set_int(r, atoi(argv[9]));

                r = config_setting_lookup(elem, "f");
                config_setting_set_float(r, atof(argv[10]));
                r = config_setting_lookup(elem, "la");
                config_setting_set_float(r, atof(argv[11]));
                r = config_setting_lookup(elem, "th");
                config_setting_set_float(r, atof(argv[12]));
                r = config_setting_lookup(elem, "down");
                config_setting_set_float(r, atof(argv[13]));
                r = config_setting_lookup(elem, "up");
                config_setting_set_float(r, atof(argv[14]));

                config_write_file(&cfg, "config.cfg");
            }
        }
    }

    if (!strcmp("up", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.analog");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);

            r = config_setting_lookup(elem, "s");
            config_setting_set_int(r, 1);
        }
        config_write_file(&cfg, "config.cfg");
    }

    if (!strcmp("down", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.analog");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);

            r = config_setting_lookup(elem, "s");
            config_setting_set_int(r, 0);
        }
        config_write_file(&cfg, "config.cfg");
    }

    if (!strcmp("auto", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.analog");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            config_setting_t *elem = config_setting_get_elem(setting, i);

            r = config_setting_lookup(elem, "th");
            config_setting_set_float(r, 0.0);
        }
        config_write_file(&cfg, "config.cfg");
    }

    if (!strcmp("noauto", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.analog");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            config_setting_t *elem = config_setting_get_elem(setting, i);

            r = config_setting_lookup(elem, "th");
            config_setting_set_float(r, 99.0);
        }
        config_write_file(&cfg, "config.cfg");
    }
}

int dealDomain(int argc, char *argv[]) {
    char *usage = "使用方法:\n"
                  "\tdc domain show\n"
                  "\tdc domain add\n"
                  "\tdc domain delete\n"
                  "\tdc domain set\n";
    if (argc == 2) {
        printf("%s\n", usage);
        return 1;
    }

    if (!strcmp("show", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.domain");
        int length = config_setting_length(setting);

        printf("号\t线路\t序号\t控时\t软压\n");

        for (int index = 0; index < length; ++index) {
            int n, l, i, otime, s;
            config_setting_t *r = config_setting_get_elem(setting, index);
            config_setting_lookup_int(r, "n", &n);
            config_setting_lookup_int(r, "l", &l);
            config_setting_lookup_int(r, "i", &i);
            config_setting_lookup_int(r, "ot", &otime);
            config_setting_lookup_int(r, "s", &s);
            printf("%d\t%d\t%d\t%d\t%d\n", n, l, i, otime, s);
        }
    }

    if (!strcmp("add", argv[2])) {
        if (argc == 3) {
            printf("号\t线路\t序号\t控时\t软压\n");
            return 0;
        }
        if (argc == 8) {
            config_setting_t *array = config_lookup(&cfg, "go.domain");
            config_setting_t * group, * r;

            group = config_setting_add(array, NULL, CONFIG_TYPE_GROUP);

            r = config_setting_add(group, "n", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[3]));
            r = config_setting_add(group, "l", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[4]));
            r = config_setting_add(group, "i", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[5]));
            r = config_setting_add(group, "ot", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[6]));
            r = config_setting_add(group, "s", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[7]));

            config_write_file(&cfg, "config.cfg");
            return 0;
        }

        printf("参数数量错误\n");
        return 0;
    }

    if (!strcmp("delete", argv[2])) {
        if (argc == 4) {
            config_setting_t *setting = config_lookup(&cfg, "go.domain");
            int length = config_setting_length(setting);

            for (int i = 0; i < length; ++i) {
                int n = 0;
                config_setting_t *r = config_setting_get_elem(setting, i);
                config_setting_lookup_int(r, "n", &n);

                if (n == atoi(argv[3])) {
                    config_setting_remove_elem(setting, i);
                    config_write_file(&cfg, "config.cfg");
                }
            }
        }
        return 0;
    }

    if (!strcmp("set", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.domain");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);
            config_setting_lookup_int(elem, "n", &n);

            if (n == atoi(argv[3])) {
                r = config_setting_lookup(elem, "n");
                config_setting_set_int(r, atoi(argv[3]));
                r = config_setting_lookup(elem, "l");
                config_setting_set_int(r, atoi(argv[4]));
                r = config_setting_lookup(elem, "i");
                config_setting_set_int(r, atoi(argv[5]));
                r = config_setting_lookup(elem, "ot");
                config_setting_set_int(r, atoi(argv[6]));
                r = config_setting_lookup(elem, "s");

                config_write_file(&cfg, "config.cfg");
            }
        }
    }

    if (!strcmp("up", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.domain");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);

            r = config_setting_lookup(elem, "s");
            config_setting_set_int(r, 1);
        }
        config_write_file(&cfg, "config.cfg");
    }

    if (!strcmp("down", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.domain");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);

            r = config_setting_lookup(elem, "s");
            config_setting_set_int(r, 0);
        }
        config_write_file(&cfg, "config.cfg");
    }
}

int dealRemote(int argc, char *argv[]) {
    char *usage = "使用方法:\n"
                  "\tdc remote show\n"
                  "\tdc remote add\n"
                  "\tdc remote delete\n"
                  "\tdc remote set\n";
    if (argc == 2) {
        printf("%s\n", usage);
        return 1;
    }

    if (!strcmp("show", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.remote");
        int length = config_setting_length(setting);

        printf("号\t类型\t遥信一\t遥信二\n");

        for (int index = 0; index < length; ++index) {
            int n, t, l0, l1;
            config_setting_t *r = config_setting_get_elem(setting, index);
            config_setting_lookup_int(r, "n", &n);
            config_setting_lookup_int(r, "t", &t);
            config_setting_lookup_int(r, "l0", &l0);
            config_setting_lookup_int(r, "l1", &l1);
            printf("%d\t%d\t%d\t%d\n", n, t, l0, l1);
        }
    }

    if (!strcmp("add", argv[2])) {
        if (argc == 3) {
            printf("号\t类型\t遥信一\t遥信二\n");
            return 0;
        }
        if (argc == 7) {
            config_setting_t *array = config_lookup(&cfg, "go.remote");
            config_setting_t * group, * r;

            group = config_setting_add(array, NULL, CONFIG_TYPE_GROUP);

            r = config_setting_add(group, "n", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[3]));
            r = config_setting_add(group, "t", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[4]));
            r = config_setting_add(group, "l0", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[5]));
            r = config_setting_add(group, "l1", CONFIG_TYPE_INT);

            config_write_file(&cfg, "config.cfg");
            return 0;
        }

        printf("参数数量错误\n");
        return 0;
    }

    if (!strcmp("delete", argv[2])) {
        if (argc == 4) {
            config_setting_t *setting = config_lookup(&cfg, "go.remote");
            int length = config_setting_length(setting);

            for (int i = 0; i < length; ++i) {
                int n = 0;
                config_setting_t *r = config_setting_get_elem(setting, i);
                config_setting_lookup_int(r, "n", &n);

                if (n == atoi(argv[3])) {
                    config_setting_remove_elem(setting, i);
                    config_write_file(&cfg, "config.cfg");
                }
            }
        }
        return 0;
    }

    if (!strcmp("set", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.remote");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);
            config_setting_lookup_int(elem, "n", &n);

            if (n == atoi(argv[3])) {
                r = config_setting_lookup(elem, "n");
                config_setting_set_int(r, atoi(argv[3]));
                r = config_setting_lookup(elem, "t");
                config_setting_set_int(r, atoi(argv[4]));
                r = config_setting_lookup(elem, "l0");
                config_setting_set_int(r, atoi(argv[5]));
                r = config_setting_lookup(elem, "l1");

                config_write_file(&cfg, "config.cfg");
            }
        }
    }
}

int dealProtect(int argc, char *argv[]) {
    char *usage = "使用方法:\n"
                  "\tdc protect show\n"
                  "\tdc protect add\n"
                  "\tdc protect delete\n"
                  "\tdc protect set\n";

    if (argc == 2) {
        printf("%s\n", usage);
        return 1;
    }

    if (!strcmp("show", argv[2])) {
        config_setting_t *setting = config_lookup(&cfg, "go.protect");
        int length = config_setting_length(setting);

        printf("号\t开关\t板\t路\t时间\t轮次\t控时\t控\t分合\t阀值\n");

        for (int index = 0; index < length; ++index) {
            int n, s, b, l, t, roll, ot, c, w;
            double th;
            config_setting_t *r = config_setting_get_elem(setting, index);
            config_setting_lookup_int(r, "n", &n);
            config_setting_lookup_int(r, "s", &s);
            config_setting_lookup_int(r, "b", &b);
            config_setting_lookup_int(r, "l", &l);
            config_setting_lookup_int(r, "t", &t);
            config_setting_lookup_int(r, "r", &roll);
            config_setting_lookup_int(r, "ot", &ot);
            config_setting_lookup_int(r, "c", &c);
            config_setting_lookup_int(r, "w", &w);
            config_setting_lookup_float(r, "th", &th);
            printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%5.2f\n", n, s, b, l, t, roll, ot, c, w, th);
        }
    }

    if (!strcmp("add", argv[2])) {
        if (argc == 3) {
            printf("号\t开关\t板\t路\t时间\t轮次\t控时\t控\t分合\t阀值\n");
            return 0;
        }
        if (argc == 13) {
            config_setting_t *array = config_lookup(&cfg, "go.protect");
            config_setting_t * group, * r;

            group = config_setting_add(array, NULL, CONFIG_TYPE_GROUP);

            r = config_setting_add(group, "n", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[3]));
            r = config_setting_add(group, "s", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[4]));
            r = config_setting_add(group, "b", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[5]));
            r = config_setting_add(group, "l", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[6]));
            r = config_setting_add(group, "t", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[7]));
            r = config_setting_add(group, "r", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[8]));
            r = config_setting_add(group, "ot", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[9]));
            r = config_setting_add(group, "c", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[10]));
            r = config_setting_add(group, "w", CONFIG_TYPE_INT);
            config_setting_set_int(r, atoi(argv[11]));
            r = config_setting_add(group, "th", CONFIG_TYPE_FLOAT);
            config_setting_set_float(r, atof(argv[12]));

            config_write_file(&cfg, "config.cfg");
            return 0;
        }

        printf("参数数量错误\n");
        return 0;
    }

    if (!strcmp("delete", argv[2])) {
        if (argc == 4) {
            config_setting_t *setting = config_lookup(&cfg, "go.protect");
            int length = config_setting_length(setting);

            for (int i = 0; i < length; ++i) {
                int n = 0;
                config_setting_t *r = config_setting_get_elem(setting, i);
                config_setting_lookup_int(r, "n", &n);

                if (n == atoi(argv[3])) {
                    config_setting_remove_elem(setting, i);
                    config_write_file(&cfg, "config.cfg");
                }
            }
        }
        return 0;
    }

    if (!strcmp("set", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.protect");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);
            config_setting_lookup_int(elem, "n", &n);

            if (n == atoi(argv[3])) {
                r = config_setting_lookup(elem, "n");
                config_setting_set_int(r, atoi(argv[3]));
                r = config_setting_lookup(elem, "s");
                config_setting_set_int(r, atoi(argv[4]));
                r = config_setting_lookup(elem, "b");
                config_setting_set_int(r, atoi(argv[5]));
                r = config_setting_lookup(elem, "l");
                config_setting_set_int(r, atoi(argv[6]));
                r = config_setting_lookup(elem, "t");
                config_setting_set_int(r, atoi(argv[7]));
                r = config_setting_lookup(elem, "r");
                config_setting_set_int(r, atoi(argv[8]));
                r = config_setting_lookup(elem, "ot");
                config_setting_set_int(r, atoi(argv[9]));
                r = config_setting_lookup(elem, "c");
                config_setting_set_int(r, atoi(argv[10]));
                r = config_setting_lookup(elem, "w");
                config_setting_set_int(r, atoi(argv[11]));
                r = config_setting_lookup(elem, "th");
                config_setting_set_float(r, atof(argv[12]));

                config_write_file(&cfg, "config.cfg");
            }
        }
    }

    if (!strcmp("up", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.protect");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);
            config_setting_lookup_int(elem, "n", &n);

            r = config_setting_lookup(elem, "s");
            config_setting_set_int(r, 1);
        }
        config_write_file(&cfg, "config.cfg");
    }

    if (!strcmp("down", argv[2])) {

        config_setting_t *setting = config_lookup(&cfg, "go.protect");
        config_setting_t * r;
        int length = config_setting_length(setting);

        for (int i = 0; i < length; ++i) {
            int n = 0;
            config_setting_t *elem = config_setting_get_elem(setting, i);
            config_setting_lookup_int(elem, "n", &n);

            r = config_setting_lookup(elem, "s");
            config_setting_set_int(r, 0);
        }
        config_write_file(&cfg, "config.cfg");
    }
}

int main(int argc, char *argv[]) {

    config_setting_t *setting;

    config_init(&cfg);

    if (!config_read_file(&cfg, "./config.cfg")) {
        printf("e\n");
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return (EXIT_FAILURE);
    }

    //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr)); //把一段内存区的内容全部设置为0
    client_addr.sin_family = AF_INET;    //internet协议族
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);//INADDR_ANY表示自动获取本机地址
    client_addr.sin_port = htons(0);    //0表示让系统自动分配一个空闲端口
    //创建用于internet的流协议(TCP)socket,用client_socket代表客户机socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        printf("Create Socket Failed!\n");
        exit(1);
    }

    //把客户机的socket和客户机的socket地址结构联系起来
    if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
        printf("Client Bind Port Failed!\n");
        exit(1);
    }

    //设置一个socket地址结构server_addr,代表服务器的internet地址, 端口
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_aton("127.0.0.1", &server_addr.sin_addr) == 0) { //服务器的IP地址来自程序的参数
        printf("Server IP Address Error!\n");
        exit(1);
    }

    server_addr.sin_port = htons(8888);
    socklen_t server_addr_length = sizeof(server_addr);
    //向服务器发起连接,连接成功后client_socket代表了客户机和服务器的一个socket连接
    if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
        printf("Can Not Connect To % s!\n", argv[1]);
        exit(1);
    }


    if (argc == 1) {
        printf(" % s\n", usage);

        _cmd cmd;
        cmd.type = 2;
        int f = send(client_socket, (void *)&cmd, sizeof(cmd), 0);

        return 0;
    }

    if (!strcmp("an", argv[1])) {
        if (argc != 3) {
            printf("usage\n");
        }
        _cmd cmd;
        cmd.type = 3;
        cmd.board = atoi(argv[2]);
        printf("%d\n", cmd.board);

        printf("\033[2J");
        while (1) {
            printf("\033[20A");
            fflush(stdout);
            send(client_socket, (void *)&cmd, sizeof(cmd), 0);
            _YcValue yc;
            recv(client_socket, (void *)&yc, sizeof(yc), 0);
            printf("=================================ANALOG=================================\n");
            printf("Ua:%5.2f\tUb:%5.2f\tUc:%5.2f\tUd:%5.2f\tF:%5.2f\n", yc.Ua, yc.Ub, yc.Uc, yc.Ud, yc.Hz1);
            printf("Ia:%6.3f\tIb:%6.3f\tIc:%6.3f\tP:%8.3f\tQ:%8.3f\t\n", yc.Ia1, yc.Ib1, yc.Ic1, yc.P1, yc.Q1);
            printf("Ia:%6.3f\tIb:%6.3f\tIc:%6.3f\tP:%8.3f\tQ:%8.3f\t\n", yc.Ia2, yc.Ib2, yc.Ic2, yc.P2, yc.Q2);
            printf("Ia:%6.3f\tIb:%6.3f\tIc:%6.3f\tP:%8.3f\tQ:%8.3f\t\n", yc.Ia3, yc.Ib3, yc.Ic3, yc.P3, yc.Q3);
            printf("Ia:%6.3f\tIb:%6.3f\tIc:%6.3f\tP:%8.3f\tQ:%8.3f\t\n", yc.Ia4, yc.Ib4, yc.Ic4, yc.P4, yc.Q4);

            usleep(200 * 1000);
        }
    }

    if (!strcmp("re", argv[1])) {
        if (argc != 3) {
            printf("usage\n");
        }
        _cmd cmd;
        cmd.type = 4;

        printf("\033[2J");
        while (1) {
            printf("\033[50A");
            fflush(stdout);
            send(client_socket, (void *)&cmd, sizeof(cmd), 0);
            _Soe soe;
            recv(client_socket, (void *)&soe, sizeof(soe), 0);
            printf("序号\t状态\t点号\t类型\t精时\t时间\n");
            for (int i = 1; i < 25; ++i) {
                int index = (soe.info.head + YX_SIZE - i) % YX_SIZE;
                struct timeval t;
                t.tv_sec = soe.unit[index].time / 1000;
                t.tv_usec = soe.unit[index].time % 1000;

                printf("[#%02d]\t%d-%d\t%04d\t%02d\t%04d\t%s", i, soe.unit[index].state[0], soe.unit[index].state[0], soe.unit[index].number, soe.unit[index].type, t.tv_usec, ctime((time_t *)&t));
            }
            usleep(200 * 1000);
        }
    }

    if (!strcmp("adapter", argv[1])) {
        _cmd cmd;

        cmd.type = 1;
        cmd.u = atof(argv[2]);
        cmd.i = atof(argv[3]);
        cmd.p = atof(argv[4]);
        cmd.q = atof(argv[5]);

        char buf[32];
        int f = send(client_socket, (void *)&cmd, sizeof(cmd), 0);

        int length = recv(client_socket, (void *)buf, sizeof(buf), 0);
        printf("buf = % d\n", (int)*buf);
        close(client_socket);

        return 0;
    }

    if (!strcmp("analog", argv[1])) {
        dealAnalog(argc, argv);
        return 0;
    }
    if (!strcmp("domain", argv[1])) {
        dealDomain(argc, argv);
        return 0;
    }
    if (!strcmp("remote", argv[1])) {
        dealRemote(argc, argv);
        return 0;
    }
    if (!strcmp("protect", argv[1])) {
        dealProtect(argc, argv);
        return 0;
    }
    return 0;
}
