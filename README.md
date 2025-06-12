# local-abacus-c-s

本地c/s的 “计算器服务”，客户端发送计算请求，服务端计算后响应

## 依赖

计算模块使用 gnu `libmatheval-dev` ，需要安装该库  
`apt-get install libmatheval-dev`

## 编译
```bash
gcc abacus.c -o abacus -lm -lmatheval
gcc abacus-cli.c -o abacus-cli -lm -lmatheval
```

## 容器

可在 (这里)[https://hub.docker.com/repository/docker/amyway/optimized_ssh_app/general] 下载容器  
`docker run -d -p 2222:22 --name ssh-and-abacus amyway/optimized_ssh_app` 绑定到宿主机的2222端口，使用ssh连接
