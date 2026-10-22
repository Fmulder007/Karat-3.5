/*
 * ============================================================
 * ДОРАБОТКА ДЛЯ ПРОЕКТА KARAT-3.5
 * Специализированная библиотека внешней EEPROM AT24C32
 * Версия доработки: 2.0.0
 * Дата: 2026-10-22
 *
 * Что изменено:
 * 1. Оставлены только нужные скетчу функции: конструктор,
 *    initialize(), readByte(), readEE() и writeEE() вместе с
 *    внутренними обработчиками структур.
 * 2. Удалены неиспользуемые функции пакетного чтения/записи:
 *    readBytes(), writeBytes(), readBuffer(), writeBuffer(),
 *    writePage() и связанные константы.
 * 3. writeEE_mem() читает каждый существующий байт EEPROM и
 *    записывает его только в случае отличия от новых данных.
 * 4. Формат и адресация данных в EEPROM не изменены.
 * 5. Обработка ошибок чтения I2C отдельно не добавлена по
 *    согласованию с пользователем.
 *
 * Изменённые и добавленные участки отмечены комментариями.
 * ============================================================
 */

/**************************************************************************//**
 * \brief EEPROM 24C32 / 24C64 library for Arduino
 * \author Copyright (C) 2012  Julien Le Sech - www.idreammicro.com
 * \version 1.0 (базовая версия; проектная доработка Karat-3.5 — 2.0.0)
 * \date 20120218 (базовая версия)
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

#include <Arduino.h>
#include <Wire.h>
#include "Eeprom24C32_64.h"

// >>> ДОРАБОТКА KARAT-3.5 v2.0.0: компактная реализация конструктора.
Eeprom24C32_64::Eeprom24C32_64(byte deviceAddress)
    : m_deviceAddress(deviceAddress)
{
}
// <<< КОНЕЦ ДОРАБОТКИ KARAT-3.5 v2.0.0: конструктор.

// >>> СОХРАНЁННАЯ ФУНКЦИЯ ПО СОГЛАСОВАНИЮ: инициализация шины I2C.
// В проекте Karat-3.5 шина также используется другими библиотеками.
void Eeprom24C32_64::initialize()
{
    Wire.begin();
}
// <<< КОНЕЦ ОТМЕЧЕННОГО УЧАСТКА initialize().

/**************************************************************************//**
 * Чтение одного байта по адресу EEPROM.
 * Используется непосредственно скетчем для расчёта контрольной суммы,
 * а также внутренним механизмом сравнения перед выборочной записью.
 ******************************************************************************/
// >>> КОМПАКТНАЯ РЕАЛИЗАЦИЯ KARAT-3.5 v2.0.0: чтение одного байта.
byte Eeprom24C32_64::readByte(word address)
{
    Wire.beginTransmission(m_deviceAddress);
    Wire.write((byte)(address >> 8));
    Wire.write((byte)(address & 0xFF));
    Wire.endTransmission();

    Wire.requestFrom(m_deviceAddress, (byte)1);

    byte data = 0;
    if (Wire.available())
    {
        data = Wire.read();
    }
    return data;
}
// <<< КОНЕЦ ОТМЕЧЕННОГО УЧАСТКА readByte().

// >>> КОМПАКТНАЯ РЕАЛИЗАЦИЯ KARAT-3.5 v2.0.0: чтение блока через readByte().
void Eeprom24C32_64::readEE_mem(word address, void *pData, int size)
{
    byte *p = (byte *)pData;
    for (int i = 0; i < size; ++i)
    {
        *p++ = readByte(address++);
    }
}
// <<< КОНЕЦ ОТМЕЧЕННОГО УЧАСТКА readEE_mem().

// >>> НАЧАЛО ДОРАБОТКИ KARAT-3.5 v2.0.0: выборочная запись структур и переменных.
// Каждый входной байт обрабатывается ровно один раз: чтение старого значения,
// сравнение и запись только при отличии. Адрес и указатель продвигаются
// на каждом шаге, включая случай совпадающих байтов, поэтому данные не пропускаются.
void Eeprom24C32_64::writeEE_mem(word address, const void *pData, int size)
{
    const byte *p = (const byte *)pData;

    for (int i = 0; i < size; ++i)
    {
        const byte newData = *p++;
        const byte oldData = readByte(address);

        if (oldData != newData)
        {
            writeByteRaw(address, newData);
        }

        ++address;
    }
}
// <<< КОНЕЦ ДОРАБОТКИ KARAT-3.5 v2.0.0: выборочная запись.

// >>> НАЧАЛО ДОРАБОТКИ KARAT-3.5 v2.0.0: физическая запись одного изменённого байта.
// Это внутренний метод. До вызова он уже сравнен с текущим содержимым EEPROM.
void Eeprom24C32_64::writeByteRaw(word address, byte data)
{
    Wire.beginTransmission(m_deviceAddress);
    Wire.write((byte)(address >> 8));
    Wire.write((byte)(address & 0xFF));
    Wire.write(data);
    Wire.endTransmission();

    // Время внутреннего цикла записи EEPROM (tWR), как в оригинальной библиотеке.
    delay(10);
}
// <<< КОНЕЦ ДОРАБОТКИ KARAT-3.5 v2.0.0: физическая запись.
