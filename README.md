# m5
IoT Code Examples for M5Stack, M5Stick C Plus, ATOM Lite  

## �T���v���W


## �{�R���e���c�̍ŐV�łƃ_�E�����[�h���@  

    �ŐV�ł̕ۑ���  
    - https://bokunimo.net/git/esp32c3/
    
    �_�E�����[�h���@(GitHub����)
    - git clone https://bokunimo.net/git/esp32c3/

## ��ȃt�H���_���A�v���O������

�{���|�W�g���Ɏ��^������ȃv���O�����̃t�H���_���A�t�@�C�����̈ꗗ�������܂��B  

|�t�H���_�� |���e                                                  |
|-----------|------------------------------------------------------|
|atom       |M5Stack�� ATOM / ATOM Lite / �ʏ�� ESP32-WROOM-32 �p |
|core       |M5Stack�� Core �p                                     |
|stick_cplus|M5Stack�� M5Stick C Plus �p                           |
|pictures   |�֘A�摜�t�@�C��                                      |
|tools      |�֘A�c�[��                                            |
|LICENSE    |���C�Z���X���e(MIT���C�Z���X:�v�����\���E���ۏ�)      |

|�t�H���_�� |���e                                                                               |
|-----------|-----------------------------------------------------------------------------------|
|ex00_hello |Arduino IDE �C���X�g�[����̓���m�F�p�v���O����                                   |
|ex01_led   |LED����p�v���O�����BHTTP�T�[�o�@�\�ɂ��u���E�U���琧��\                      |
|ex02_sw    |�����{�^���̑��M�v���O�����Bex01_led��LED�̐����LINE�ւ̑��M���\                |
|ex03_lum   |�Ɠx�Z���T�̑��M�v���O�����B�Ɠx�l���N���E�h(Ambient)�ɑ��M���O���t�����\        |
|ex04_lcd   |���^�t���ւ̕\���v���O�����Bex02�A03�A05�̑��M�f�[�^�̕\�����\                   |
|ex05_hum   |���x�{���x�Z���T�̑��M�v���O�����B�Ƃ��イ�̕����ɐݒu����΋��Z���̊Ď����\   |
|ex06_pir   |�l���Z���T�E���j�b�g�iPIR Motion Sensor�j���g����Wi-Fi�l���Z���T�p�v���O����       |
|ex07_ir_in |�ԊO�������R���E���j�b�g�iIR Unit�j�Ń����R���R�[�h���擾����v���O����            |
|ex08_ir_out|�ԊO�������R���E���j�b�g�iIR Unit�j���g����Wi-Fi�ԊO���E�����R���p�v���O����       |
|ex09_talk  |Wi-Fi�R���V�F���W�F�m�����A�i�E���X�S���n�������� AquesTalk Pico LSI ATP3012�p     |
|ex10_cam   |Wi-Fi�R���V�F���W�F�m�J�����S���nGrove - Serial Camera Kit�p                       |

## Arduino IDE �p�� ESP32 �J�����̃Z�b�g�A�b�v  

ESP32�J���{�[�h�i ESP32-WROOM-32 ���ځj�Ŏg�p����ꍇ�A���L�̎菇�ŊJ�������Z�b�g�A�b�v���A
�uatom�v�t�H���_���̃T���v�����g�p���Ă��������B  

	atom �t�H���_ : ESP32�J���{�[�h ESP32-WROOM-32 �Ή��T���v��  

1. Arduino IDE (https://www.arduino.cc/en/software/) ���C���X�g�[������B  
2. Arduino IDE ���N�����A[�t�@�C��]���j���[����[���ݒ�]���J���A�u�ǉ��̃{�[�h�}�l�[�W����URL�v�̗��ɉ��L�́u����v��ǉ�����B  

    �����  
    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json  

    �J���r���  
    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json  

    �Q�l����  
    - https://github.com/espressif/arduino-esp32 (�ŐV���)  
    - https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html (��񂪌Â��ꍇ������̂Œ���)  

3. [�c�[��]���j���[����[�{�[�h]����{�[�h�}�l�[�W�����J���A�������Ɂuesp32�v����͌�Aesp32 by Espressif Systems ���C���X�g�[������B  

4. [�c�[��]���j���[����[�{�[�h]�� ESP32C3 DEev Module ��I������B  

by bokunimo.net(https://bokunimo.net/)  
- �u���O (https://bokuniomo.net/blog/)  
- �J�e�S��ESP (https://bokunimo.net/blog/category/esp/)  
