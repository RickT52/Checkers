#pragma once
#include <stdlib.h>

typedef int8_t POS_T; // Определение типа POS_T как 8-битного целого числа (для хранения координат)

struct move_pos
{
    POS_T x, y;             // Начальная позиция (откуда) хода
    POS_T x2, y2;           // Конечная позиция (куда) хода
    POS_T xb = -1, yb = -1; // Позиция побитой шашки (по умолчанию -1, что указывает на отсутствие побитой шашки)

    // Конструктор для создания объекта move_pos с начальной и конечной позициями
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Второй конструктор, позволяющий также указать побитую шашку
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Оператор сравнения для проверки равенства двух переменных move_pos
    bool operator==(const move_pos& other) const
    {
        // Возвращает true, если все компоненты объекта совпадают с другим объектом
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // Оператор неравенства, использующий оператор равенства
    bool operator!=(const move_pos& other) const
    {
        // Возвращает true, если текущий объект не равен другому объекту
        return !(*this == other);
    }
};