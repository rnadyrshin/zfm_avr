//------------------------------------------------------------------------------
// This is Open source software. You can place this code on your site, but don't
// forget a link to my YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// ��� ����������� ����������� ���������������� ��������. �� ������ ���������
// ��� �� ����� �����, �� �� �������� ������� ������ �� ��� YouTube-����� 
// "����������� � ���������" https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// �����: �������� ������ / Nadyrshin Ruslan
//------------------------------------------------------------------------------
#ifndef _MAIN_H
#define _MAIN_H


// ����� ���������� �� ����� � ��
#define ZFM_LedState_Port               PORTB
#define ZFM_LedState_DDR                DDRB
#define ZFM_LedState_Mask               (1 << 5)
// ����� ���������� �� ����� � ��
#define ZFM_LedRed_Port                 PORTC
#define ZFM_LedRed_DDR                  DDRC
#define ZFM_LedRed_Mask                 (1 << 4)
// ����� ���������� �� ����� � ��
#define ZFM_LedGreen_Port               PORTC
#define ZFM_LedGreen_DDR                DDRC
#define ZFM_LedGreen_Mask               (1 << 3)
// ����� ���������� �� ����� � ��
#define ZFM_LedBlue_Port                PORTC
#define ZFM_LedBlue_DDR                 DDRC
#define ZFM_LedBlue_Mask                (1 << 2)
// ����� ��� �������� � ����� �������� ����� ����������
#define ZFM_EnrollNewFinger_Port        PORTC
#define ZFM_EnrollNewFinger_DDR         DDRC
#define ZFM_EnrollNewFinger_Pin         PINC
#define ZFM_EnrollNewFinger_Mask        (1 << 0)
// ����� ��� ������� ���������� �������� ����������
#define ZFM_ClearFingerLib_Port         PORTD
#define ZFM_ClearFingerLib_DDR          DDRD
#define ZFM_ClearFingerLib_Pin          PIND
#define ZFM_ClearFingerLib_Mask         (1 << 3)


#endif