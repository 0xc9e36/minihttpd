# studyHttpd
一个简单的多线程http服务器,  用C语言写的, 支持php.  

#  支持   
目录显示   
html   
php(需要php-fpm)   
GET/POST方法   
...   

#  TODO
recv, send的封装   
内存管理   
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

