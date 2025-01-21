#pragma once
#include <stdlib.h>

typedef int8_t POS_T; // ����������� ���� POS_T ��� 8-������� ������ ����� (��� �������� ���������)

struct move_pos
{
    POS_T x, y;             // ��������� ������� (������) ����
    POS_T x2, y2;           // �������� ������� (����) ����
    POS_T xb = -1, yb = -1; // ������� ������� ����� (�� ��������� -1, ��� ��������� �� ���������� ������� �����)

    // ����������� ��� �������� ������� move_pos � ��������� � �������� ���������
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // ������ �����������, ����������� ����� ������� ������� �����
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // �������� ��������� ��� �������� ��������� ���� ���������� move_pos
    bool operator==(const move_pos& other) const
    {
        // ���������� true, ���� ��� ���������� ������� ��������� � ������ ��������
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // �������� �����������, ������������ �������� ���������
    bool operator!=(const move_pos& other) const
    {
        // ���������� true, ���� ������� ������ �� ����� ������� �������
        return !(*this == other);
    }
};