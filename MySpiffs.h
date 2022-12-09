/**
   MySpiffs.h
   @autor    Bruno Merz

   @version  2.0
   @created  30.12.2019
   @updated  18.04.2021

*/

#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LITTLEFS.h> 


/**
   standard HTML site as long as we don't have a special one uploaded
*/
const char INI_Setup_html[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="de">
  <head>
    <title>INI Wortuhr</title>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <link rel="Icon" type="image/ico" href="favicon.ico">
      <style>
        body {
        background-color: #D0D0D4;
        font-family: Arial, Helvetica, sans-serif; text-align: center; }
        h1 {
        color: navy;
        margin-left: 20px;
        font-size: 150%
        }
        .button {
        border-top: 1px solid #96d1f8;
        background: #65a9d7;
        background: -webkit-gradient(linear, left top, left bottom, from(#3e779d), to(#65a9d7));
        background: -webkit-linear-gradient(top, #3e779d, #65a9d7);
        background: -moz-linear-gradient(top, #3e779d, #65a9d7);
        background: -ms-linear-gradient(top, #3e779d, #65a9d7);
        background: -o-linear-gradient(top, #3e779d, #65a9d7);
        padding: 5.5px 11px;
        -webkit-border-radius: 8px;
        -moz-border-radius: 8px;
        border-radius: 8px;
        -webkit-box-shadow: rgba(0,0,0,1) 0 1px 0;
        -moz-box-shadow: rgba(0,0,0,1) 0 1px 0;
        box-shadow: rgba(0,0,0,1) 0 1px 0;
        text-shadow: rgba(0,0,0,.4) 0 1px 0;
        color: white;
        font-size: 16px;
        font-family: Georgia, serif;
        text-decoration: none;
        vertical-align: middle;
        }
        .button:hover {
        border-top-color: #28597a;
        background: #28597a;
        color: #ffffff;
        }
        .button:active {
        border-top-color: #bf157e;
        background: #bf157e;
        }
        .buttonr {color: #F59FA0;}
        input[type="text"] {
        margin: 0;
        height: 28px;
        background: white;
        font-size: 16px;
        appearance: none;
        box-shadow: none;
        border-radius: 5px;
        -webkit-border-radius: 5px;
        -moz-border-radius: 5px;
        -webkit-appearance: none;
        border: 1px solid black;
        border-radius: 10px;
        }
        input[type="text"]:focus {
        outline: none;
        }
        textarea {
        max-width: 99%;
        width: 900px;
        line-height:1.4;
        border: 1px solid black;
        border-radius: 10px;
        font-family: Courier, "Lucida Console", monospace;
        background-color: white;
        resize: none;
        }
      </style>
  </head>
  <body>
    Datei hochladen:
    <form action="#" onsubmit="return uploadfile(this);" enctype="multipart/form-data" method="post" name="fileinfo">
      <input name="file" size="50" type="file">
      <br>
      <input class="button" value="Senden" type="submit">
    </form>
    
    
    <script>

function httpGet ( theReq )
{
  var theUrl = "/edit?" + theReq + "&version=" + Math.random() ;
  var xhr = new XMLHttpRequest() ;
  xhr.onreadystatechange = function() {
    if ( xhr.readyState == XMLHttpRequest.DONE )
    {
      //resultstr.value = xhr.responseText ;
    }
  }
  xhr.open ( "GET", theUrl, false ) ;
  xhr.send() ;
}
function uploadfile ( theForm )
{
  var oData, oReq ;
  oData = new FormData ( fileinfo ) ;
  oReq = new XMLHttpRequest() ;
  oReq.open ( "POST", "/upload", true ) ;
  oReq.send ( oData ) ;
  return false ;
}
  </script>
  </body>
</html>
)";

/**
 * Class definition
 */
class MySpiffs {
  public:
    static MySpiffs* getInstance();

    fs::File    openFile(String filename, char *mode);
    size_t  readBytes(uint8_t *buffer, int len);
    int     readFile(String filename, char *buffer);
    int     readFileS(String filename, String *buffer);
    void    writeFile(const uint8_t *buf, size_t len);
    void    writeFile(char *buf);
    void    closeFile(void);
    void    remove(String filename);
    void    format(void);
    bool    exists(String filename);
    int     fileSize(String filename);
    uint32_t getFreeSpace(void);
    String  getHtmlFilename(String html, bool checkUpdate);
    void    reloadResource(void);
    void    cleanup(void);
    void    writeLog(int32_t val, boolean withDateTime=true);
    void    writeLog(char *txt, boolean withDateTime=true);
    void    writeLog(const __FlashStringHelper*, boolean withDateTime=true);
    String  getState(void);
   

private:
  MySpiffs();
  ~MySpiffs();
  
  static MySpiffs *instance;

  int     _fileSize;
  fs::File    fsUploadFile;
  fs::File    logHandle;
  boolean _reloadResource;

};
