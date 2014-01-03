export LUAJIT_LIB=/usr/local/lib
export LUAJIT_INC=/usr/local/include/luajit-2.0

sh configure --prefix=/home/fang/bin-nginx \
 --add-module=/home/fang/nginx_php_module \
 --add-module=/home/fang/ngx_devel_kit-0.2.19\
 --add-module=/home/fang/lua-nginx-module-0.9.2 \
# --with-debug
