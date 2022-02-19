/********************************

		シーン基本

 ********************************/

#include "Scene.h"
#include "Engine.h"


namespace sys
{

uint32_t	Scene::common_counter;			// 汎用カウンタ
uint32_t	Scene::key_status;				// キー入力状態


/****************************************
    assetファイル読み込み
			引数	name = ファイル名
			戻り値	データ
					size：データサイズ
 ****************************************/
std::unique_ptr<uint8_t>	Scene::load_asset(const char* name, size_t* size)
{
	LOGI("Load Asset \"%s\"", name);

	AAsset*	_asset = AAssetManager_open(Engine::app->activity->assetManager, name, AASSET_MODE_BUFFER);

	assert(_asset);

	size_t	_size = AAsset_getLength(_asset);		// ファイルサイズ
	uint8_t*	_buf = new uint8_t[_size];			// データバッファ

	AAsset_read(_asset, _buf, _size);				// データ読み込み
	AAsset_close(_asset);
	if ( size ) {
		*size = _size;
	}
	return	std::unique_ptr<uint8_t>(_buf);
}


/**********
    稼働
 **********/
void	Scene::update_manager(android_input_buffer* _input)
{
	common_counter++;								// 汎用カウンタ
	rand();

	key_status = 0;									// キー入力
	if ( _input ) {
		for (size_t i = 0; i < _input->keyEventsCount; i++) {
			GameActivityKeyEvent*	_event = &_input->keyEvents[i];

			if ( (_event->keyCode == AKEYCODE_BACK) && (_event->action == 0) ) {
				key_status |= KEY_BACK;				// バックキー
			}
		}
		android_app_clear_key_events(_input);
	}
}

}
