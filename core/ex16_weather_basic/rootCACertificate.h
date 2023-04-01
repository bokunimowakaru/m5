const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDdzCCAl+gAwIBAgIBADANBgkqhkiG9w0BAQsFADBdMQswCQYDVQQGEwJKUDEl\n"\
"MCMGA1UEChMcU0VDT00gVHJ1c3QgU3lzdGVtcyBDTy4sTFRELjEnMCUGA1UECxMe\n"\
"U2VjdXJpdHkgQ29tbXVuaWNhdGlvbiBSb290Q0EyMB4XDTA5MDUyOTA1MDAzOVoX\n"\
"DTI5MDUyOTA1MDAzOVowXTELMAkGA1UEBhMCSlAxJTAjBgNVBAoTHFNFQ09NIFRy\n"\
"dXN0IFN5c3RlbXMgQ08uLExURC4xJzAlBgNVBAsTHlNlY3VyaXR5IENvbW11bmlj\n"\
"YXRpb24gUm9vdENBMjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANAV\n"\
"OVKxUrO6xVmCxF1SrjpDZYBLx/KWvNs2l9amZIyoXvDjChz335c9S672XewhtUGr\n"\
"zbl+dp+++T42NKA7wfYxEUV0kz1XgMX5iZnK5atq1LXaQZAQwdbWQonCv/Q4EpVM\n"\
"VAX3NuRFg3sUZdbcDE3R3n4MqzvEFb46VqZab3ZpUql6ucjrappdUtAtCms1FgkQ\n"\
"hNBqyjoGADdH5H5XTz+L62e4iKrFvlNVspHEfbmwhRkGeC7bYRr6hfVKkaHnFtWO\n"\
"ojnflLhwHyg/i/xAXmODPIMqGplrz95Zajv8bxbXH/1KEOtOghY6rCcMU/Gt1SSw\n"\
"awNQwS08Ft1ENCcadfsCAwEAAaNCMEAwHQYDVR0OBBYEFAqFqXdlBZh8QIH4D5cs\n"\
"OPEK7DzPMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3\n"\
"DQEBCwUAA4IBAQBMOqNErLlFsceTfsgLCkLfZOoc7llsCLqJX2rKSpWeeo8HxdpF\n"\
"coJxDjrSzG+ntKEju/Ykn8sX/oymzsLS28yN/HH8AynBbF0zX2S2ZTuJbxh2ePXc\n"\
"okgfGT+Ok+vx+hfuzU7jBBJV1uXk3fs+BXziHV7Gp7yXT2g69ekuCkO2r1dcYmh8\n"\
"t/2jioSgrGK+KwmHNPBqAbubKVY8/gA3zyNs8U6qtnRGEmyR7jTV7JqR50S+kDFy\n"\
"1UkC9gLl9B/rfNmWVan/7Ir5mUf/NVoCqgTLiluHcSmRvaS0eg29mvVXIwAHIRc/\n"\
"SjnRBUkLp7Y3gaVdjKozXoEofKd9J+sAro03\n"\
"-----END CERTIFICATE-----\n";

// 証明書はブラウザ機能で容易に取得できる。
// ①HTTPSで対象サイトにアクセス
// ②ブラウザのURL入力欄の錠前アイコンから「接続がセキュリティで保護されています」を選択し、
// 　ポップ画面の右上の証明書アイコンをクリックすると「証明書ビューア」が開く。
// ③詳細に切り替えると「証明書の階層」が表示されるので、取得したい証明書を選択してから
// 　エクスポートで出力できる。
// 　今回は最も上位のルートCA証明書を選んだ。(他を選んでも、同サイトの証明書は得られる)
//
// Base64変換はWindowsの機能で実施する。
// ①上記で保存した証明書をWindowsで開く
// ②「詳細」タブ→「ファイルにコピー」→「Base 64」を選択して保存する。

/*
	証明書を確認するとき（安全）：
    client.setCACert(rootCACertificate);    // ルートCA証明書を設定
    
    証明書を確認しないとき（有効期限切れ時などの動作確認用）：
    client.setInsecure();                   // サーバ証明書を確認しない
*/

/*
取得時先URL：
https://www.jma.go.jp/bosai/forecast/data/forecast/130000.json

更新ファイル：
rootCACertificate.h

使用箇所：
\m5\core\ex16_weather
\m5\core\ex16_weather_basic
\m5\core2\ex16_weather

*/