# nginx\_upstream\_jvm\_route #

---


This module achieves session stickiness with the session cookie. If the session is not in the cookie or URL, the module will be a normal Round-Robin upstream module.

## INSTALLATION ##
```
    cd nginx-0.7.59 # or whatever
    patch -p0 < /path/to/this/directory/jvm_route.patch
```
compile nginx with the following addition option:
```
    --add-module=/path/to/this/directory
```
## EXAMPLE ##
1.For resin
```
upstream backend {
    server 192.168.0.100 srun_id=a;
    server 192.168.0.101 srun_id=b;
    server 192.168.0.102 srun_id=c;
    server 192.168.0.103 srun_id=d;

    jvm_route $cookie_JSESSIONID;
}
```
[detail](http://code.google.com/p/nginx-upstream-jvm-route/wiki/nginx_with_resin)

2.For tomcat
```
upstream backend {
    server 192.168.0.100 srun_id=a;
    server 192.168.0.101 srun_id=b;
    server 192.168.0.102 srun_id=c;
    server 192.168.0.103 srun_id=d;

    jvm_route $cookie_JSESSIONID reverse;
}
```
[detail](http://code.google.com/p/nginx-upstream-jvm-route/wiki/nginx_with_tomcat)

3. A simple jave test page

[here](http://code.google.com/p/nginx-upstream-jvm-route/wiki/simple_java_test_page)

## DIRECTIVES ##

> ### jvm\_route ###

> syntax: jvm\_route $cookie\_COOKIE`[`|session\_url`]` `[`reverse`]`

> default: none

> context: upstream

> description:

> '$cookie\_SESSION\_COOKIE' specifies the session cookie name(0.7.24+). 'session\_url' specifies a different session name in the URL when the client does not accept a cookie. The session name is case-insensitive. In this module, if it does not find the session\_url, it will use the session cookie name instead. So if the session name in cookie is the name with its in URL, you don't need give the session\_url name.

> With scanning this cookie, the module will send the request to right backend server. As far as I know, the resin's srun\_id name is in the head of cookie. For example, requests with cookie value 'a`*``*``*`' are always sent to the server with the srun\_id of 'a'. But tomcat's JSESSIONID is opposite, which is like '`*``*``*`.a'. The parameter of 'reverse' specifies the cookie scanned from tail to head.

> If the request fails to be sent to the chosen backend server, It will try another server with the Round-Robin mode until all the upstream servers tried. The directive proxy\_next\_upstream can specify in what cases the request will be transmitted to the next server. If you want to force the session sticky, you can set 'proxy\_next\_upstream off'.

> ### server ###

> Every syntax is the same with the official directive except the parameter of 'srun\_id' which identified the backend JVM's name by cookie. The default srun\_id's value is 'a';


## NOTE ##

This is a third-party module. And you need careful test before using this module in your production environment.

Questions/patches may be directed to Weibin Yao, yaoweibin@gmail.com.