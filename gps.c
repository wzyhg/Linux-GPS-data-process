#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
  /* 示例1：$GPGGA,213035.000,3447.5084,N,11339.3580,E,1,07,1.6,97.9,M,-18.0,M,,0000*43
               $GPGLL,3447.5084,N,11339.3580,E,213035.000,A*38
               $GPGSA,A,3,16,30,13,31,23,06,03,,,,,,2.8,1.6,2.3*3E
               $GPRMC,213035.000,A,3447.5084,N,11339.3580,E,0.22,341.45,141111,,*0D
               $GPVTG,341.45,T,,M,0.22,N,0.4,K*63

         时间：2021年04月01日，21时30分35.000秒
         位置：东经113度39分3580秒，北纬34度47分5084秒 
         方位角：341度45分
         速度：0.22公里/小时
         高度：-18.0米
     
     */  
// 函数声明
int parse_gpgga(const char* data, char* time, char* latitude, char* lat_dir, char* longitude, char* lon_dir, char* altitude);
int parse_gprmc(const char* data, char* date, char* speed, char* course);
void convert_time_format(const char* nmea_time, char* formatted_time);
void convert_date_format(const char* nmea_date, char* formatted_date);
void convert_coordinate(const char* nmea_coord, char* degree, char* minute, char* second, const char* direction);
int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <data csv file> <result csv file>\n", argv[0]);
        return -1;
    }

    int fd_data = open(argv[1], O_RDONLY);
    if (fd_data < 0) {
        printf("can not open file %s\n", argv[1]);
        perror("open");
        return -1;
    } else {
        printf("data file fd = %d\n", fd_data);
    }

    int fd_result = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_result < 0) {
        printf("can not create file %s\n", argv[2]);
        perror("open");
        close(fd_data);
        return -1;
    } else {
        printf("result file fd = %d\n", fd_result);
    }

    // 读取数据文件
    char buffer[81920];
    ssize_t bytes_read = read(fd_data, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        //close(fd_data);
        //close(fd_result);
        return -1;
    }
    buffer[bytes_read] = '\0';

    // 解析GPS数据
    char time[16], latitude[16], lat_dir[2], longitude[16], lon_dir[2], altitude[16];
    char date[16], speed[16], course[16];
    char formatted_time[32], formatted_date[32];
    char lat_degree[8], lat_minute[8], lat_second[8];
    char lon_degree[8], lon_minute[8], lon_second[8];
    char year[3], month[3], day[3];
    char hour[3], minute[3], second[8];
    // 按行分割数据
    char* line = strtok(buffer, "$");
    int line_count = 0;
    int line_count1 = 0;
    while (line != NULL) {
        
        if (strstr(line, "GPGGA") != NULL) {
            if (parse_gpgga(line, time, latitude, lat_dir, longitude, lon_dir, altitude) == 0) {
                convert_time_format(time, formatted_time);
                convert_coordinate(latitude, lat_degree, lat_minute, lat_second, lat_dir);
                convert_coordinate(longitude, lon_degree, lon_minute, lon_second, lon_dir);
                line_count = 1;
            }
        }  
        else if (strstr(line, "GPRMC") != NULL) {
            if (parse_gprmc(line, date, speed, course) == 0) {
                convert_date_format(date, formatted_date);
               line_count1 = 2;
            }
        }
        line = strtok(NULL, "$");//
    if (line_count ==1 && line_count1 ==2 ){//只有处理了一组GPRMC和GPGGA数据才会打印出来
    // 生成结果字符串
    char result[1024];
    strncpy(month, formatted_date + 2, 2); month[2] = '\0';
    strncpy(year, formatted_date + 4, 2); year[2] = '\0';
    strncpy(hour, formatted_time, 2); hour[2] = '\0';
    strncpy(minute, formatted_time + 2, 2); minute[2] = '\0';
    strncpy(second, formatted_time + 4, 7); second[7] = '\0';
    strncpy(day, formatted_date, 2); day[2] = '\0';
    snprintf(result, sizeof(result),
             "时间:20%s年%s月%s日,%s时%s分%s秒\n"
             "位置：东经%s度%s分%s秒,北纬%s度%s分%s秒\n"
             "方位角：%s度\n"
             "速度：%s公里/小时\n"
             "高度：%s米\n"
             "\n",
            year, month, day,  // 年月日
             hour, minute, second,  // 时分秒
             lon_degree, lon_minute, lon_second,                      // 经度
             lat_degree, lat_minute, lat_second,                      // 纬度
             course, speed, altitude);

    // 写入结果文件
    ssize_t bytes_written = write(fd_result, result, strlen(result));
    if (bytes_written < 0) {
        perror("write");
    } else {
       // printf("结果已写入文件: %s\n", argv[2]);
    }
    // 在控制台显示结果
    printf("%s", result);
    
    //line_count =0;
    line_count1 = 0;
    line_count =0;
    }
    
    // printf("总共处理了 %d 行数据\n", line_count);
    }
    // 关闭文件
    close(fd_data);
    close(fd_result);

    return 0;
}


// 解析GPGGA语句
int parse_gpgga(const char* data, char* time, char* latitude, char* lat_dir, char* longitude, char* lon_dir, char* altitude) {
     if (data == NULL) return -1;
    
    //printf("解析GPGGA: %s\n", data);
    
    char copy[256];
    strncpy(copy, data, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';
    
    char* tokens[20];
    int token_count = 0;
    char* current = copy;
    
    // 手动分割，不使用strtok
    for (int i = 0; i < 20; i++) {
        char* comma = strchr(current, ',');
        if (comma != NULL) {
            *comma = '\0';  // 临时替换逗号为结束符
        }
        
        tokens[token_count++] = current;
       // printf("字段[%d]: %s\n", token_count - 1, current);
        
        if (comma == NULL) break;
        current = comma + 1;
    }
    
    //printf("总共 %d 个字段\n", token_count);
    
    if (token_count < 15) return -1;
    
    // 复制数据
    strncpy(time, tokens[1], 15); time[15] = '\0';
    strncpy(latitude, tokens[2], 15); latitude[15] = '\0';
    strncpy(lat_dir, tokens[3], 1); lat_dir[1] = '\0';
    strncpy(longitude, tokens[4], 15); longitude[15] = '\0';
    strncpy(lon_dir, tokens[5], 1); lon_dir[1] = '\0';
    strncpy(altitude, tokens[11], 15); altitude[15] = '\0';
    
    return 0;
}

// 解析GPRMC语句
int parse_gprmc(const char* data, char* date, char* speed, char* course) {
     if (data == NULL) {
        printf("错误: GPRMC输入数据为NULL\n");
        return -1;
    }
    
   // printf("解析GPRMC: %s\n", data);
    
    char copy[256];
    strncpy(copy, data, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';
    
    char* tokens[20];
    int token_count = 0;
    char* current = copy;
    
    // 手动分割逗号
    for (int i = 0; i < 20; i++) {
        char* comma = strchr(current, ',');
        if (comma != NULL) {
            *comma = '\0';  // 临时替换逗号为结束符
        }
        
        tokens[token_count++] = current;
       // printf("GPRMC字段[%d]: %s\n", token_count - 1, current);
        
        if (comma == NULL) break;
        current = comma + 1;
    }
    
    //printf("GPRMC总共 %d 个字段\n", token_count);
    
    if (token_count < 10) {
        printf("错误: GPRMC需要至少10个字段\n");
        return -1;
    }
    
    strncpy(date, tokens[9], 15); date[15] = '\0';
    strncpy(speed, tokens[7], 15); speed[15] = '\0';
    strncpy(course, tokens[8], 15); course[15] = '\0';
    
    //printf("GPRMC解析成功: 日期=%s, 速度=%s, 方位角=%s\n", date, speed, course);
    
    return 0;
}

// 转换时间格式 HHMMSS.SSS -> HH时MM分SS.SSS秒
void convert_time_format(const char* nmea_time, char* formatted_time) {
    if (strlen(nmea_time) >= 6) {
        char hour[3] = {nmea_time[0], nmea_time[1], '\0'};
        char minute[3] = {nmea_time[2], nmea_time[3], '\0'};
        char second[8] = {0};
        strncpy(second, nmea_time + 4, 7);
        snprintf(formatted_time, 32, "%s%s%s", hour, minute, second);
    }
}

// 转换日期格式 DDMMYY -> YY年MM月DD日
void convert_date_format(const char* nmea_date, char* formatted_date) {
    if (strlen(nmea_date) >= 6) {
        char day[3] = {nmea_date[0], nmea_date[1], '\0'};
        char month[3] = {nmea_date[2], nmea_date[3], '\0'};
        char year[3] = {nmea_date[4], nmea_date[5], '\0'};
        snprintf(formatted_date, 32, "%s%s%s", year, month, day);
    }
}

// 转换坐标格式 DDDMM.MMMM -> 度分秒
void convert_coordinate(const char* nmea_coord, char* degree, char* minute, char* second, const char* direction) {
    if (strlen(nmea_coord) >= 7) {
        // 提取度
        int deg_len = (direction[0] == 'E' || direction[0] == 'W') ? 3 : 2;
        strncpy(degree, nmea_coord, deg_len);
        degree[deg_len] = '\0';
        
        // 提取分和秒
        char remaining[16];
        strncpy(remaining, nmea_coord + deg_len, sizeof(remaining) - 1);
        remaining[sizeof(remaining) - 1] = '\0';

       char* dot_pos = strchr(remaining, '.');
    if (dot_pos != NULL) {
        // 提取整数分钟
        int min_len = dot_pos - remaining;
        strncpy(minute, remaining, min_len);
        minute[min_len] = '\0';
        
        // 小数部分直接为秒
         strncpy(second, dot_pos + 1, 7);
         second[7] = '\0';
        
    } else {
        // 没有小数点，只有整数分钟
        strcpy(minute, remaining);
        strcpy(second, "0.0");
    }
    }
}