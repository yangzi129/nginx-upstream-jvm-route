
```
<%@ page language="java" import="java.util.*" pageEncoding="UTF-8"%>

<html>
  <head>
    </head>
      <body>
       $your_jvm_name
       <br />
       <%out.print(request.getSession()) ;%> <br />
       <%out.println(request.getHeader("Cookie")); %>
      </body>
 </html>
```