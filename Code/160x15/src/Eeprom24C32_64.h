/*
 * ============================================================
 * ДОРАБОТКА ДЛЯ ПРОЕКТА KARAT-3.5
 * Специализированная библиотека внешней EEPROM AT24C32
 * Версия доработки: 2.0.0
 * Дата: 2026-10-22
 *
 * Назначение версии 2.0.0:
 * 1. Оставлены только операции, используемые скетчем 160x14.ino:
 *    initialize(), readByte(), readEE() и writeEE().
 * 2. Удалены неиспользуемые пути пакетного чтения и записи.
 * 3. writeEE() выполняет выборочную запись: каждый байт сначала
 *    читается и сравнивается; совпадающие байты не перезаписываются.
 * 4. Сохранены адресация EEPROM и формат сохранённых данных.
 * 5. Ошибки чтения I2C отдельно не обрабатываются по согласованию.
 *
 * Основа: Eeprom24C32_64, Julien Le Sech, 2012.
 * Изменения для Karat-3.5 отмечены комментариями в коде.
 * ============================================================
 */

/**************************************************************************//**
 * \brief EEPROM 24C32 / 24C64 library for Arduino
 * \author Copyright (C) 2012  Julien Le Sech - www.idreammicro.com
 * \version 1.0 (базовая версия; проектная доработка Karat-3.5 — 2.0.0)
 * \date 20120203 (базовая версия)
 *
 * This file is part of the EEPROM 24C32 / 24C64 library for Arduino.
 *
 * This library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 ******************************************************************************/

#ifndef Eeprom24C32_64_h
#define Eeprom24C32_64_h

#include <Arduino.h>

// >>> ДОРАБОТКА KARAT-3.5 v2.0.0: минимальный API, необходимый скетчу 160x14.ino.
class Eeprom24C32_64
{
public:
    // Совместимость с существующим объявлением: Eeprom24C32_64 AT24C32(0x50).
    Eeprom24C32_64(byte deviceAddress);

    // Оставлено по просьбе пользователя: запускает шину Wire (Wire.begin()).
    // В текущем скетче напрямую не вызывается.
    void initialize();

    // Используется скетчем для побайтной проверки контрольной суммы.
    byte readByte(word address);

    // Чтение переменной или структуры с указанного адреса EEPROM.
    template<typename T>
    void readEE(word address, T &data)
    {
        readEE_mem(address, &data, sizeof(data));
    }

    // Запись переменной или структуры; неизменённые байты пропускаются.
    template<typename T>
    void writeEE(word address, const T &data)
    {
        writeEE_mem(address, &data, sizeof(data));
    }

private:
    byte m_deviceAddress;

    void readEE_mem(word address, void *pData, int size);
    void writeEE_mem(word address, const void *pData, int size);

    // >>> ДОРАБОТКА KARAT-3.5 v2.0.0: низкоуровневая безусловная запись одного байта.
    // Вызывается только после того, как writeEE_mem обнаружила отличие.
    void writeByteRaw(word address, byte data);
    // <<< КОНЕЦ ДОРАБОТКИ KARAT-3.5 v2.0.0.
};
// <<< КОНЕЦ ДОРАБОТКИ KARAT-3.5 v2.0.0: минимальный API.

#endif // Eeprom24C32_64_h
