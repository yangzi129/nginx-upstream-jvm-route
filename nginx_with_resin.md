# Nginx's configure #
```
upstream backend {
    server 192.168.0.100 srun_id=a;
    server 192.168.0.101 srun_id=b;
   
    jvm_route $cookie_JSESSIONID;
}
```
# Resin's configure #
**For all resin servers**
```
    <server id="a" address="192.168.0.100" port="8080">
    <http id="" port="80"/>
    </server>
    <server id="b" address="192.168.0.101" port="8080">
    <http id="" port="80"/>
    </server>
```

And start each resin instances like this:

**server a**

```
shell $> /usr/local/resin/bin/httpd.sh -server a start
```

**server b**

```
shell $> /usr/local/resin/bin/httpd.sh -server b start
```