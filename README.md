# VSP4L_EYEAPP

Vision support program on linux

## 実行環境 / Build&Run enviroment

* Ubuntu 18.04LTS以降   
  * DebianベースのLinux系OSなら多少の手直しで動作するはず
  * Ubuntu16.04LTSだとlibturbojpeg等の依存関係をcmakeが検出できない場合があるがそれらを解消すれば実行できるはず。
* GLESを有効にする

## ビルド方法 / How To Build

### 依存ライブラリをインストール / Install dependancies

`sudo apt install libpng libjpeg libturbojpeg libudev`

本リポジトリのサブモジュールとして`glfw`, `imgui`, `libyuv`, `rapidjson`を含む。

### ビルド手順 / How to Build

* リポジトリをクローンする(サブモジュールを含むので注意)
* インストールしていない場合はclangをインストールする。
* c/c++のコンパイラーとしてclangを使うように設定する。
   `sudo update-alternatives --config cc`でcコンパイラーとしてclangを選択する。  
   `sudo update-alternatives --config c++`でc++コンパイラーとしてclangを選択する。  
   note: なぜかgccだと純粋仮想クラスがリンクできない。
* クローンしたディレクトリ内へ移動する。
* buildディレクトリを作成する。
   ```
   mkdir build
   ```
* cmakeを実行する。
   ```
   cmake ../src
   ```
* makeを実行して実行ファイルを生成する。  
   buildディレクトリ内に`EyeApp`実行ファイルが生成される。
   ```
   make
   ```

## 実行方法
  
* UVCカメラ等のV4L2での映像入力に対応した機器を接続する。
* 接続した機器に対応するデバイスファイルを確認する(ここでは/dev/video0と仮定する)。
* コマンドラインから`EyeApp`実行ファイルを実行する。   
   `./EyeApp`

### コマンドラインオプション

* -e / --debug_exit_esc  
   デバッグ用にESCキーでアプリを終了できるようにする
* -f / --debug_show_fps  
   デバッグ用に映像描画フレームレートを表示する
* -d / --device  
   使用するデバイスファイルを指定する。デフォルトは"/dev/video0”
* -u / udmabuf   
   V4L2機器から映像データを受け取る際に使うUDMABUFのデバイスファイル名を指定する。 デフォルトは"/dev/udmabuf0"
* -n / buf_nums  
   V4L2機器から映像データを受け取る際に使うバッファの数を指定する。デフォルトは"4"
* -w / --width   
   V4L2機器から受け取る映像データの幅を指定する。デフォルトは"1920"
* -h / --height   
   V4L2機器から受け取る映像データの高さを指定する。デフォルトは"1080"

## キー操作

* 上矢印または下矢印長押し
  拡大縮小モードへ
  * 拡大縮小モードでは上矢印で拡大、下矢印で縮小、一定時間放置で通常モードへ
* 左矢印または右矢印長押し  
  輝度調整モードへ
  * 輝度調整モードでは右矢印で明るく、左矢印で暗く
* 上矢印または下矢印ダブルタップ  
  映像効果ローテーション
* 左矢印または右矢印ダブルタップ  
  フリーズモードON/OFF
* 上矢印または下矢印と左矢印または右矢印を同時長押し
  OSDモードへ
* ENTER
  OSDモードで決定
* ESC  
  アプリ終了(-e / --debug_exit_escオプション指定時)

## FIXME

* ショートオプションが正常に動かない
* なにの制限か不明ながら、2Kや4Kの大きな画面だと全画面表示にならず画面の一部分にだけ表示される。
* 今のところEyeAppファイルが存在するディレクトリに移動してから実行しないと正常にアイコンとフォントを読み込めない。


