# transactions-on-nvram

modify top lines of Makefile to switch between OSX, pmdk, and ucx

careful, there are hardcoded paths:
> find . -type f | xargs grep user1

careful, there is one hardcoded ip address:
> find . -type f | xargs grep 192.168

on server:
UCX_NET_DEVICES=mlx4_0:1 UCX_TLS=rc ./red-server

on client:
UCX_NET_DEVICES=mlx4_0:1 UCX_TLS=rc ./red-client

for pmdk:
> module load clang/6.0.0 pmdk/1.3.1

for ucx:
> module load clang/6.0.0 pmdk/1.3.1 ucx/1.3.0
