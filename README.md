## studyHttpd
一个简单的多线程http服务器, 用C语言写的, 使用fastcgi协议与php-fpm通信.

## 支持
目录显示    
html静态页面    
php(需要php-fpm)     
GET/POST请求    

##  演示
安装 php-fpm  :  sudo apt-get install php5-fpm   
设置监听端口   :   listen =127.0.0.1:9000   

运行    
 ![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/start.png)  

目录显示效果 :  
 ![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/dir.png)    
 
html静态页面    
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/html.png)  

GET请求   
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/get.png)

POST上传文件   
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/file.png)   
![image](https://github.com/tw1996/studyHttpd/blob/master/readme-img/file_post.png)
