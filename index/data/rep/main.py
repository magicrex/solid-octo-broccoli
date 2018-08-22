#!/usr/bin/python
#coding:utf-8

import urllib3
import re
import os
from bs4 import BeautifulSoup

#打开页面
def OpenPage(url):
    http = urllib3.PoolManager()
    r = http.request('GET',url)
    return r.data

def ParseMainPage(page):
    soup = BeautifulSoup(page,"html.parser")
    ListChapter = soup.find_all('dl',{'class':'links'})
    urlList = []
    for item in ListChapter:
        url = "http://www.cplusplus.com"  + item.a["href"]
        urlList.append(url)
    return urlList


def ParseMinorPage(url):
    page=OpenPage(url)
    title = url[35:]
    title_path = "../input/" + title
    isExists = os.path.exists(title_path)
    if not isExists:
        os.makedirs(title_path)
    title_path = title_path + "index.html"
    f = open(title_path , 'w')
    f.write(page)
    f.close()



def run():
    page = OpenPage("http://www.cplusplus.com/reference/stl/")
    GetUrl = ParseMainPage(page)
    for item in GetUrl:
        ParseMinorPage(item)
    print "end"


#主函数
if __name__ == "__main__":
    run()
