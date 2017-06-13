# studyHttpd
一个简单的http服务器,  C语言写的, epoll+非阻塞io+线程池结构, 支持php.  

#  支持   
目录显示   
html   
php解析(需要php-fpm)     
GET/POST方法     
...   

#  TODO    
https   
内存管理    
日志管理      
超时管理    
...  

#  演示   
ubuntu 安装 php-fpm :  sudo apt-get install php5-fpm   
改配置文件设置监听端口 :   listen =127.0.0.1:9000  

启动  
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/start.png)    
设置根目录为htdocs, 浏览器访问  
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/dir.png)   
看看  html页面   
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/html.png)   
在url上试一下get方法  
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/get.png)    
然后试试post,  来个form表单   
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/post_html.png)    
跳转后     
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/post_php.png)    
上传个文件试试    
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/file.png)    
打印   
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/file_post.png)     
ci框架(具体没测试)       
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/ci.png) 


#  参考资料   
TinyHttpd    
深入理解计算机系统(第三部分)    
http://andylin02.iteye.com/blog/648412      
http://lifeofzjs.com/blog/2015/05/16/how-to-write-a-server/      
http://thinkwp.cn/?p=775
