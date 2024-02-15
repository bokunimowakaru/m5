#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
extern Ink_Sprite InkPageSprite;

void ink_test_cls(){
    /*  全画面消去を分割して実行する(本来は不要な処理のはず)
        画面消去できない事象が発生するようになったので対策。
        deltaで消去エリアの大きさを設定。
        200(全画面対象)だと消去できないことがある。
    */
    int delta = 25;
    for(int y=0;y<200;y+=delta){
        InkPageSprite.FillRect(0,y,200,delta,1); // White
        InkPageSprite.pushSprite();              // e-paperに描画
        InkPageSprite.FillRect(0,y,200,delta,0); // Black
        InkPageSprite.pushSprite();              // e-paperに描画
        InkPageSprite.FillRect(0,y,200,delta,1); // White
        InkPageSprite.pushSprite();              // e-paperに描画
    }
}

void ink_test(){
    M5.update();                                // M5Stack用IO状態の更新
    while(!M5.M5Ink.isInit()) delay(3000);      // Inkの初期化状態確認
    if(M5.BtnUP.isPressed()){
        InkPageSprite.creatSprite(0,0,200,200,0);
    //  InkPageSprite.drawFullBuff(PageBuf);    // RTCメモリから画像読み込み
    /*  本来は下記で消去できるはず
    //  InkPageSprite.clear();
    //  InkPageSprite.pushSprite();             // e-paperに描画
        原因不明だが消去できないことがあるので、lib_ink_test.ino内の下記を使用 */
        while(!M5.BtnUP.isReleased()) M5.update();
        ink_test_cls();                             // 暫定対策(画面消去)
        lineGraphSetSprite(&InkPageSprite, 16, 0, 100); // 棒グラフ描画用の設定
        ink_print_init(&InkPageSprite);             // テキスト表示用 ink_print
        lineGraphCls();                             // グラフ画面の罫線描画
        lineGraphRedraw();                          // 過去グラフの再描画
        ink_println("Example 5 HUM");           // タイトルの描画
        ink_printPos(160);                      // テキスト文字位置を上から160に
        ink_println("Screen Cleared (UP btn)"); // Inkへメッセージを表示
        sleep();                                // PageBufを更新してスリープ
    }
    if(M5.BtnMID.isPressed()){
        M5.M5Ink.clear();                       // Inkを消去
        InkPageSprite.creatSprite(0,0,200,200,0);
        InkPageSprite.drawFullBuff(PageBuf);    // RTCメモリから画像読み込み
        ink_print_init(&InkPageSprite);         // テキスト表示用 ink_print
        ink_printPos(160);                      // テキスト文字位置を上から160に
        ink_println("Reset Ink (MID btn)");     // Inkへメッセージを表示
        ink_println("Please release it");       // Inkへメッセージを表示
        while(!M5.BtnMID.isReleased()) M5.update();
        pinMode(0, OUTPUT);
        digitalWrite(0, LOW);
        delay(3000);
        digitalWrite(0, HIGH);
        delay(100);
        while(!M5.M5Ink.isInit()) delay(3000);  // Inkの初期化状態確認
        ink_printPos(160);                      // テキスト文字位置を上から160に
        ink_println("Power is Down ("+String(batt_mv())+" mV)");
        ink_println("Please disconnect USB");   // Inkへメッセージを表示
        sleep();                                // PageBufを更新してスリープ
    }
    if(M5.BtnDOWN.isPressed()){
        InkPageSprite.creatSprite(0,0,200,200,0);
        InkPageSprite.drawFullBuff(PageBuf);    // RTCメモリから画像読み込み
        ink_print_setup(&InkPageSprite);        // テキスト表示用 ink_print
        ink_printPos(160);                      // テキスト文字位置を上から160に
        ink_println("DOWN button is pressed");  // Inkへメッセージを表示
        ink_println("Please release it");       // Inkへメッセージを表示
        while(!M5.BtnDOWN.isReleased()) M5.update();
        ink_printPos(160);                      // テキスト文字位置を上から160に
        ink_println("Power is Down ("+String(batt_mv())+" mV)");
        ink_println("Please disconnect USB");   // Inkへメッセージを表示
        delay(3000);
        memcpy(PageBuf,InkPageSprite.getSpritePtr(),200*200/8); // RTCに保存
        M5.shutdown();                          // 
    }
}
