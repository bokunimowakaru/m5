/*******************************************************************************
HTMLコンテンツ
                                          Copyright (c) 2016-2024 Wataru KUNINO
*******************************************************************************/

#define _html_size 840 + 32

String getHtml(int target, float level, float rpm, float wow, boolean pause){
    char html[_html_size],s[65],s_ip[16];
    uint32_t ip = WiFi.localIP();

    sprintf(s_ip,"%d.%d.%d.%d",
        ip & 255,
        ip>>8 & 255,
        ip>>16 & 255,
        ip>>24
    );
    snprintf(html,_html_size,"<html>\n<head>\n<title>Wi-Fi 回転数RPM計</title>\n<meta http-equiv=\"Content-type\" content=\"text/html; charset=UTF-8\">\n<meta http-equiv=\"refresh\" content=\"5;URL=http://%s/\">\n</head>\n<body>\n<h3>M5Stick STATUS</h3>\n",s_ip);
    if(wow>=100) sprintf(s,"---"); else sprintf(s,"%.2f",wow);
    snprintf(html,_html_size,"%s\n<p>Level=%.3f °</p><p>RPM=%.3f rpm</p><p>WOW=%s %%</p>",html,level,rpm,s);
    if(target<0 || pause) sprintf(s,"<p>Pause</p>"); else sprintf(s,"<p>Mode=%d</p>",target);
    snprintf(html,_html_size,"%s\n%s\n<hr>\n<h3>LCD切替</h3>\n<p>http://%s/?mode=n<br>\n</p>",html,s,s_ip);
    sprintf(s,"<form method=\"GET\" action=\"http://%s/\">",s_ip);
    snprintf(html,_html_size,"%s\n%s\n<input type=\"submit\" name=\"mode\" value=\"-1 (Pause)\">\n<input type=\"submit\" name=\"mode\" value=\"0 (水平器)\">",html,s);
    snprintf(html,_html_size,"%s\n<input type=\"submit\" name=\"mode\" value=\"1 (グラフ)\">\n<input type=\"submit\" name=\"mode\" value=\"2 (回転数)\">\n<input type=\"submit\" name=\"mode\" value=\"4 (照度-回転数)\">\n</form>",html);
    snprintf(html,_html_size,"%s\n<hr>\n<h3>CSVデータ取得</h3>\n<p><a href=\"http://%s/rpm.csv\">http://%s/rpm.csv</a><br>\n</p>",html,s_ip,s_ip);
    snprintf(html,_html_size,"%s\n</body>\n</html>",html);
    Serial.println("sizeof(html)=" + String(strlen(html)+1));
    return String(html);
}
