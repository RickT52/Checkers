#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        // Создаёт или очищает лог-файл для записи действий игры
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Функция для запуска игры в шашки
    int play()
    {
        // Запоминает время начала игры
        auto start = chrono::steady_clock::now();

        // Если это повторная игра, перезагружает логику и конфигурацию
        if (is_replay)
        {
            logic = Logic(&board, &config); // Инициализация логики с текущей конфигурацией
            config.reload(); // Перезагрузка конфигурации
            board.redraw(); // Перерисовка доски
        }
        else
        {
            board.start_draw(); // Первоначальная отрисовка доски для новой игры
        }
        is_replay = false; // Сбрасывает флаг повторной игры

        int turn_num = -1; // Счётчик ходов
        bool is_quit = false; // Флаг, указывающий, вышел ли игрок из игры
        const int Max_turns = config("Game", "MaxNumTurns"); // Получение максимального количества ходов из конфигурации

        // Цикл, который продолжается до достижения максимального количества ходов
        while (++turn_num < Max_turns)
        {
            beat_series = 0; // Сброс серии ударов
            logic.find_turns(turn_num % 2); // Находит возможные ходы для текущего игрока
            // Если нет доступных ходов, игра заканчивается
            if (logic.turns.empty())
                break;

            // Устанавливает максимальную глубину поиска для бота в зависимости от текущего игрока
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // Проверяет, является ли текущий ход ботом
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Ход игрока
                auto resp = player_turn(turn_num % 2);

                // Обработка ответа от игрока: выход, повтор, или шаг назад
                if (resp == Response::QUIT)
                {
                    is_quit = true; // Устанавливает флаг выхода
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true; // Устанавливает флаг повторной игры
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Проверяет, может ли игрок откатить ход
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback(); // Откат хода
                        --turn_num; // Уменьшает счётчик ходов
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback(); // Повторный откат для завершения текущего хода
                    --turn_num;
                    beat_series = 0; // Сброс серии ударов
                }
            }
            else
                bot_turn(turn_num % 2); // Ход бота
        }

        // Подсчёт времени игры и запись в лог
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n"; // Запись времени игры
        fout.close();

        // Обработка повторной игры или выхода
        if (is_replay)
            return play(); // Запускает игру снова
        if (is_quit)
            return 0; // Выход из игры

        // Определение результата игры
        int res = 2; // Результат по умолчанию победа белых 
        if (turn_num == Max_turns)
        {
            res = 0; // Ничья
        }
        else if (turn_num % 2)
        {
            res = 1; // Победа черного
        }
        board.show_final(res); // Показывает финальные результаты игры
        auto resp = hand.wait(); // Ожидание ответа от игрока
        if (resp == Response::REPLAY)
        {
            is_replay = true; // Устанавливает флаг повторной игры
            return play(); // Запускает игру снова
        }
        return res; // Возвращает результат игры
    }

private:
    // Функция для обработки хода бота
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now(); // Запоминает начало хода бота

        auto delay_ms = config("Bot", "BotDelayMS"); // Получает задержку для бота
        // Создаёт новый поток для задержки хода бота
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color); // Находит лучшие ходы для бота
        th.join(); // Ожидает завершения потока задержки

        bool is_first = true; // Флаг для отслеживания первого хода
        // Выполнение хода бота
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms); // Задержка между ходами
            }
            is_first = false; // Устанавливает флаг, что первый ход завершён
            beat_series += (turn.xb != -1); // Увеличивает серию ударов, если было выполнено ударное движение
            board.move_piece(turn, beat_series); // Двигает фигуру на доске
        }

        // Запись времени хода бота в лог
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // Создание вектора для хранения доступных клеток для первого хода
        vector<pair<POS_T, POS_T>> cells;

        // Заполнение вектора доступными ходами из логики игры
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y); // Сохраняем начальную позицию ходов
        }

        // Подсветка всех доступных клеток на доске
        board.highlight_cells(cells);

        // Инициализация переменной для хранения текущего хода
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1; // Координаты текущего хода

        // Попытка сделать первый ход
        while (true)
        {
            // Получение выбранной игроком клетки
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL) // Проверка, выбрана ли клетка
                return get<0>(resp); // Если нет, вернуть соответствующий ответ

            // Получение координат выбранной клетки
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

            bool is_correct = false; // Флаг для проверки корректности хода
            // Проверка, можно ли выполнить данный ход
            for (auto turn : logic.turns)
            {
                // Если выбранная клетка соответствует началу одного из возможных ходов
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true; // Ход корректен
                    break;
                }
                // Если перехода на выбранную клетку соответствует возможный ход
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn; // Сохраняем информацию о текущем ходе
                    break;
                }
            }

            // Если удалось найти корректный ход, выходим из цикла
            if (pos.x != -1)
                break;

            // Если ход некорректен
            if (!is_correct)
            {
                // Если до этого для активного элемента была выбрана клетка
                if (x != -1)
                {
                    board.clear_active(); // Убираем активный элемент
                    board.clear_highlight(); // Убираем подсветку
                    board.highlight_cells(cells); // Подсвечиваем доступные клетки вновь
                }
                // Сброс координат
                x = -1;
                y = -1;
                continue; // Переход к следующему циклу
            }

            // Обновляем координаты выбранной клетки
            x = cell.first;
            y = cell.second;
            board.clear_highlight(); // Убираем подсветку
            board.set_active(x, y); // Устанавливаем активный элемент на выбранной клетке

            // Создание нового вектора для хранения доступных ходов из текущей позиции
            vector<pair<POS_T, POS_T>> cells2;
            // Подсвечиваем возможные клетки для следующего хода
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y) // Если текущая клетка соответствует истории ходов
                {
                    cells2.emplace_back(turn.x2, turn.y2); // Добавляем целевую клетку для хода
                }
            }
            board.highlight_cells(cells2); // Подсвечиваем возможные клетки для последующего движения
        }

        // Убираем подсветку и активный элемент после завершения выбора
        board.clear_highlight();
        board.clear_active();

        // Выполняем перемещение выбранной шашки, если она не была побита
        board.move_piece(pos, pos.xb != -1);

        // Если шашка не была побита, возврат успешного ответа
        if (pos.xb == -1)
            return Response::OK;

        // Начинаем обработку возможных серий побитий

        beat_series = 1; // Счетчик для количества последующих побитий
        while (true)
        {
            // Находим доступные ходы для побитой шашки
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats) // Если больше нет доступных побитий
                break; // Выходим из цикла

            // Создание вектора для хранения клеток, доступных для побитий
            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2); // Добавляем целевые клетки для побитий
            }

            // Подсветка возможных клеток для побитий
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2); // Устанавливаем активный элемент на клетку, откуда побивание

            // Цикл ожидания подтверждения следующего хода побития
            while (true)
            {
                auto resp = hand.get_cell(); // Получаем выбор игрока для следующего хода
                if (get<0>(resp) != Response::CELL) // Если выбор не клетка, возвращаем соответствующий ответ
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) }; // Получаем координаты выбранной клетки

                bool is_correct = false; // Флаг для проверки корректности хода
                // Проверка, является ли выбранная клетка доступным ходом для побития
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second) // Если клетка соответствует доступным клеткам для побития
                    {
                        is_correct = true; // Ход корректен
                        pos = turn; // Сохраняем новую позицию согласно текущему ходу
                        break;
                    }
                }
                if (!is_correct) // Если ход некорректен, продолжаем
                    continue; // Вернуться в начало цикла для нового выбора

                // Убираем подсветку и активный элемент
                board.clear_highlight();
                board.clear_active();
                beat_series += 1; // Увеличиваем количество побитий
                board.move_piece(pos, beat_series); // Выполняем ход

                break; // Выход из цикла после выполнения хода
            }
        }

        return Response::OK; // Возврат успешного ответа, завершившего ход
    }
private:
    Config config; // Объект для хранения конфигурационных параметров игры, таких как настройки правил, цвета и т.д.

    Board board; // Объект для управления состоянием игровой доски и ее визуализацией на экране.

    Hand hand; // Объект, который отвечает за ввод пользователя, обработку кликов на клетках и взаимодействие с игроком.

    Logic logic; // Объект, инкапсулирующий все правила и логику игры, включая алгоритмы поиска ходов и оценку состояния игры.

    int beat_series; // Счетчик, указывающий количество последовательных побитий текущим игроком; используется для подсчета возможных дополнительных ходов.

    bool is_replay = false; // Флаг, указывающий, находится ли игра в режиме повторного прохождения (например, после завершения игры или для просмотра предыдущих действий).
};
