# Nginx's configure #
```
upstream backend {
    server 192.168.0.100 srun_id=a;
    server 192.168.0.101 srun_id=b;
   
    jvm_route $cookie_JSESSIONID reverse;
}
```
# Tomcat's configure #
**Tomcat a:**
```
<Engine name="Catalina" defaultHost="localhost" jvmRoute="a">
```
**Tomcat b:**
```
<Engine name="Catalina" defaultHost="localhost" jvmRoute="b">
```