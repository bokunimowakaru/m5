/*******************************************************************************
HTMLコンテンツ
                                          Copyright (c) 2016-2024 Wataru KUNINO
*******************************************************************************/

#define _html_size 639 + 32

String getHtml(int target, float rpm, float wow){
    char html[_html_size],s[65],s_ip[16];
    uint32_t ip = WiFi.localIP();

    sprintf(s_ip,"%d.%d.%d.%d",
        ip & 255,
        ip>>8 & 255,
        ip>>16 & 255,
        ip>>24
    );
    snprintf(html,_html_size,"<html>\n<head>\n<title>Wi-Fi 回転数RPM計</title>\n<meta http-equiv=\"Content-type\" content=\"text/html; charset=UTF-8\">\n<meta http-equiv=\"refresh\" content=\"5;URL=http://%s/\">\n</head>\n<body>\n<h3>M5Stick STATUS</h3>\n",s_ip);
    snprintf(html,_html_size,"%s\n<p>RPM=%.2f rpm</p><p>WOW=%.2f %%</p>",html,rpm,wow);
    if(target<0) sprintf(s,"<p>Pause</p>");
    if(target>=0) sprintf(s,"<p>Mode=%d</p>",target);
    snprintf(html,_html_size,"%s\n%s\n<hr>\n<h3>HTTP GET</h3>\n<p>http://%s/?mode=n<br>\n</p>",html,s,s_ip);
    sprintf(s,"<form method=\"GET\" action=\"http://%s/\">",s_ip);
    snprintf(html,_html_size,"%s\n%s\n<input type=\"submit\" name=\"mode\" value=\"-1 (Pause)\">\n<input type=\"submit\" name=\"mode\" value=\"0 (水平器)\">\n",html,s);
    snprintf(html,_html_size,"%s\n<input type=\"submit\" name=\"mode\" value=\"1 (グラフ)\">\n<input type=\"submit\" name=\"mode\" value=\"2 (回転数)\">\n</form>\n</body>\n</html>",html);
    Serial.println("sizeof(html)=" + String(strlen(html)+1));
    // sizeof(html)=567
    return String(html);
}
