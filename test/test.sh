#!/bin/bash 
 
# 参数检查 
if [ "$#" -lt 2 ]; then
    echo "用法: $0 <RTSP_URL> <并发次数> [额外FFmpeg参数]"
    echo "示例: $0 rtsp://127.0.0.1:8554/record/test1 5"
    exit 1
fi
 
RTSP_URL=$1 
CONCURRENCY=$2
 
# 并发执行FFmpeg 
for ((i=1; i<=$CONCURRENCY; i++))
do
    echo "启动进程 $i, 拉流URL: $RTSP_URL"
    nohup ./build/client "$RTSP_URL" >/dev/null 2>&1 &
done 
 
echo "已启动 $CONCURRENCY 个FFmpeg进程"