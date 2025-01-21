#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс для обработки ввода от игрока
class Hand
{
public:
    // Конструктор класса, который принимает указатель на объект доски
    Hand(Board* board) : board(board)
    {
    }

    // Метод для получения точки нажатия мыши на доске
    // Возвращает кортеж (Response, POS_T, POS_T)
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // Переменная для обработки событий SDL
        Response resp = Response::OK; // Начальное значение ответа
        int x = -1, y = -1; // Координаты мыши на экране
        int xc = -1, yc = -1; // Координаты выбранной клетки на доске

        // Бесконечный цикл для ожидания событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверка на наличие событий
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Установка флага выхода при закрытии окна
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    // Получаем координаты мыши при нажатии
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;

                    // Вычисляем индексы клетки на доске
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);

                    // Проверка на команды отката, перезапуска и выбора клетки
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // Откат хода
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // Перезапуск игры
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // Выбор клетки на доске
                    }
                    else
                    {
                        xc = -1; // Сброс индексов клетки для некорректного нажатия
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    // Обработка изменения размера окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // Сброс размеров доски
                        break;
                    }
                }
                if (resp != Response::OK) // Если ответ изменился, выходим из цикла
                    break;
            }
        }
        return { resp, xc, yc }; // Возвращаем ответ и координаты клетки
    }

    // Метод для ожидания события, возвращает ответ (Response)
    Response wait() const
    {
        SDL_Event windowEvent; // Переменная для обработки событий SDL
        Response resp = Response::OK; // Начальное значение ответа

        // Бесконечный цикл ожидания событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверка на наличие событий
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Установка флага выхода при закрытии окна
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:

                    board->reset_window_size(); // Обновление размеров доски
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    // Получаем координаты мыши при нажатии
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; // Если нажата кнопка "Перезапуск"
                }
                                        break;
                }
                if (resp != Response::OK) // Если ответ изменился, выходим из цикла
                    break;
            }
        }
        return resp; // Возвращаем ответ
    }

private:
    Board* board; // Указатель на объект доски для взаимодействия с ней
};