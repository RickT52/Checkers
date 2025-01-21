#pragma once
#include <iostream>
#include <fstream>
#include <vector>

#include "../Models/Move.h"
#include "../Models/Project_path.h"

#ifdef APPLE
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

using namespace std;

class Board
{
public:
    // Конструктор по умолчанию
    Board() = default;

    // Конструктор, устанавливающий размеры доски
    Board(const unsigned int W, const unsigned int H) : W(W), H(H)
    {
    }

    // Метод для отрисовки стартовой доски
    int start_draw()
    {
        // Инициализация SDL
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }

        // Установка размеров окна, если они не заданы
        if (W == 0  H == 0)
        {
            SDL_DisplayMode dm;
            // Получение размера экрана для создания окна
            if (SDL_GetDesktopDisplayMode(0, &dm))
            {
                print_exception("SDL_GetDesktopDisplayMode can't get desktop display mode");
                return 1;
            }
            W = min(dm.w, dm.h);
            W -= W / 15;
            H = W;
        }

        // Создание окна
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE);
        if (win == nullptr)
        {
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }

        // Создание рендерера для окна
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ren == nullptr)
        {
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }

        // Загрузка текстур для доски и фигур
        board = IMG_LoadTexture(ren, board_path.c_str());
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str());
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str());
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str());
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str());
        back = IMG_LoadTexture(ren, back_path.c_str());
        replay = IMG_LoadTexture(ren, replay_path.c_str());

        // Проверка успешности загрузки текстур
        if (!board  !w_piece  !b_piece  !w_queen  !b_queen  !back  !replay)
        {
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }

        // Получение размеров рендерера
        SDL_GetRendererOutputSize(ren, &W, &H);
        make_start_mtx(); // Создание начального состояния доски
        rerender(); // Отрисовка доски
        return 0; // Успешное завершение
    }

    // Метод для перерисовки доски и сброса состояния
    void redraw()
    {
        game_results = -1; // Сброс результата игры
        history_mtx.clear(); // Очистка истории ходов
        history_beat_series.clear(); // Очистка истории серий побитий
        make_start_mtx(); // Создание начального состояния доски
        clear_active(); // Очистка активного элемента
        clear_highlight(); // Очистка подсветки
    }

    // Метод для перемещения фигуры на доске с учетом побитой фигуры
    void move_piece(move_pos turn, const int beat_series = 0)
    {
        if (turn.xb != -1) // Если была побита фигура
        {

            mtx[turn.xb][turn.yb] = 0; // Обнуляем клетку, где находилась побитая фигура
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series); // Выполняем перемещение
    }

    // Метод для перемещения фигуры с заданными координатами
    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0)
    {
        // Проверка, что целевая клетка пуста
        if (mtx[i2][j2])
        {
            throw runtime_error("final position is not empty, can't move"); // Исключение, если конечная позиция занята
        }
        // Проверка, что начальная клетка не пуста
        if (!mtx[i][j])
        {
            throw runtime_error("begin position is empty, can't move"); // Исключение, если начальная позиция пуста
        }
        // Проверка на возможность превращения в ферзя
        if ((mtx[i][j] == 1 && i2 == 0)  (mtx[i][j] == 2 && i2 == 7))
            mtx[i][j] += 2; // Увеличиваем значение на 2 для ферзя

        mtx[i2][j2] = mtx[i][j]; // Перемещение фигуры
        drop_piece(i, j); // Удаление фигуры с начальной позиции
        add_history(beat_series); // Добавление информации о побитии в историю
    }

    // Метод для удаления фигуры с доски
    void drop_piece(const POS_T i, const POS_T j)
    {
        mtx[i][j] = 0; // Обнуляем клетку
        rerender(); // Перерисовываем доску
    }

    // Метод для превращения обычной фигуры в ферзя
    void turn_into_queen(const POS_T i, const POS_T j)
    {
        // Проверка, что в данной позиции находится фигура
        if (mtx[i][j] == 0 || mtx[i][j] > 2)
        {
            throw runtime_error("can't turn into queen in this position"); // Исключение, если превращение невозможно
        }

        mtx[i][j] += 2; // Превращение в ферзя
        rerender(); // Перерисовываем доску
    }

    // Метод для получения матрицы состояния доски
    vector<vector<POS_T>> get_board() const
    {
        return mtx; // Возвращаем матрицу
    }

    // Метод для подсветки заданных клеток
    void highlight_cells(vector<pair<POS_T, POS_T>> cells)
    {
        // Подсветка каждой заданной клетки
        for (auto pos : cells)
        {
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1; // Устанавливаем статус подсветки
        }
        rerender(); // Перерисовываем доску для отображения изменений
    }
    // Метод для очистки подсветки клеток на доске
    void clear_highlight()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            is_highlighted_[i].assign(8, 0); // Сброс статуса подсветки для всех клеток
        }
        rerender(); // Перерисовываем доску, чтобы удалить подсветку
    }

    // Метод для установки активной клетки
    void set_active(const POS_T x, const POS_T y)
    {
        active_x = x; // Запоминаем координаты активной клетки
        active_y = y;
        rerender(); // Перерисовываем доску, чтобы отобразить активную клетку
    }

    // Метод для очистки активной клетки
    void clear_active()
    {
        active_x = -1; // Сброс активной клетки
        active_y = -1;
        rerender(); // Перерисовываем доску для обновления отображения
    }

    // Метод для проверки, подсвечена ли указанная клетка
    bool is_highlighted(const POS_T x, const POS_T y)
    {
        return is_highlighted_[x][y]; // Возвращаем статус подсветки указанной клетки
    }

    // Метод для отката последних ходов
    void rollback()
    {
        auto beat_series = max(1, *(history_beat_series.rbegin())); // Получаем количество побитий из истории
        while (beat_series-- && history_mtx.size() > 1)
        {
            history_mtx.pop_back(); // Удаляем последний ход из истории
            history_beat_series.pop_back(); // Удаляем информацию о побитии
        }
        mtx = *(history_mtx.rbegin()); // Восстанавливаем состояние доски до последнего хода
        clear_highlight(); // Очищаем подсветку
        clear_active(); // Очищаем активную клетку
    }

    // Метод для отображения конечного результата игры
    void show_final(const int res)
    {
        game_results = res; // Устанавливаем результат игры
        rerender(); // Перерисовываем доску для отображения результата
    }

    // Метод для сброса размеров окна в случае их изменения
    void reset_window_size()
    {
        SDL_GetRendererOutputSize(ren, &W, &H); // Получаем новые размеры окна
        rerender(); // Перерисовываем доску с новыми размерами
    }

    // Метод для завершения работы приложения и освобождения ресурсов
    void quit()
    {
        // Освобождение загруженных текстур
        SDL_DestroyTexture(board);
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren); // Уничтожение рендерера
        SDL_DestroyWindow(win); // Уничтожение окна
        SDL_Quit(); // Завершение работы SDL
    }

    // Деструктор класса
    ~Board()
    {
        if (win) // Проверка, существует ли окно
            quit(); // Если да, вызываем метод для выхода и освобождения ресурсов
    }

private:
    // Метод для добавления состояния доски в историю
    void add_history(const int beat_series = 0)
    {
        history_mtx.push_back(mtx); // Сохраняем текущее состояние доски
        history_beat_series.push_back(beat_series); // Сохраняем информацию о побитии
    }

    // Метод для создания начальной матрицы с расстановкой фигур
    void make_start_mtx()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                mtx[i][j] = 0; // Изначально все клетки пустые
                // Расположение черных фигур
                if (i < 3 && (i + j) % 2 == 1)
                    mtx[i][j] = 2; // Черные фигуры
                // Расположение белых фигур

                if (i > 4 && (i + j) % 2 == 1)
                    mtx[i][j] = 1; // Белые фигуры
            }
        }
        add_history(); // Добавляем данное состояние в историю
    }

    // Метод для перерисовки всех текстур на доске
    void rerender()
    {
        // Очистка экрана перед новым отрисовкой
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, board, NULL, NULL); // Отрисовка фона доски

        // Отрисовка фигур на доске
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!mtx[i][j]) // Если клетка пуста, пропускаем
                    continue;
                int wpos = W * (j + 1) / 10 + W / 120; // Вычисляем положение для фигуры по оси X
                int hpos = H * (i + 1) / 10 + H / 120; // Вычисляем положение для фигуры по оси Y
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 }; // Создаем прямоугольник для фигуры

                SDL_Texture* piece_texture; // Указатель на текстуру фигуры
                // Определение текстуры в зависимости от типа фигуры
                if (mtx[i][j] == 1)
                    piece_texture = w_piece; // Белая фигура
                else if (mtx[i][j] == 2)
                    piece_texture = b_piece; // Черная фигура
                else if (mtx[i][j] == 3)
                    piece_texture = w_queen; // Белый ферзь
                else
                    piece_texture = b_queen; // Черный ферзь

                SDL_RenderCopy(ren, piece_texture, NULL, &rect); // Отрисовка фигуры
            }
        }

        // Отрисовка подсветки клеток
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0); // Установка цвета для подсветки
        const double scale = 2.5;  // Масштаб для подсветки
        SDL_RenderSetScale(ren, scale, scale); // Установка масштаба 
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!is_highlighted_[i][j]) // Если клетка не подсвечена, пропускаем
                    continue;
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale),
                              int(H / 10 / scale) }; // Создание прямоугольника для подсветки
                SDL_RenderDrawRect(ren, &cell); // Отрисовка рамки для подсветки
            }
        }

        // Отрисовка активной клетки
        if (active_x != -1) // Если активная клетка установлена
        {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0); // Установка цвета для выделения активной клетки
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale),
                                 int(W / 10 / scale), int(H / 10 / scale) }; // Прямоугольник для активной клетки
            SDL_RenderDrawRect(ren, &active_cell); // Отрисовка рамки вокруг активной клетки
        }
        SDL_RenderSetScale(ren, 1, 1); // Сброс масштаба

        // Рисуем стрелки для действий (назад и повтора)
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 }; // Создание прямоугольника для стрелки "назад"
        SDL_RenderCopy(ren, back, NULL, &rect_left); // Отрисовка стрелки "назад"

        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 }; // Создание прямоугольника для стрелки "повтор"
        SDL_RenderCopy(ren, replay, NULL, &replay_rect); // Отрисовка стрелки "повтор"

        // Отображаем результат игры победа, поражение или ничья
        if (game_results != -1) // Проверяем, был ли установлен результат игры
        {
            string result_path = draw_path; // Устанавливаем путь к текстуре результата (по умолчанию ничья)
            // Определяем путь к текстуре результата в зависимости от того, кто выиграл
            if (game_results == 1)
                result_path = white_path; // Победа белых
            else if (game_results == 2)
                result_path = black_path; // Победа черных

            // Загружаем текстуру результата
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());
            if (result_texture == nullptr) // Проверка на успешную загрузку текстуры
            {
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path); // Логирование ошибки
                return; // Выход из функции, если текстура не загружена
            }

            // Определяем прямоугольник для отображения результата на экране
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect); // Отрисовка текстуры результата
            SDL_DestroyTexture(result_texture); // Освобождение ресурса текстуры после использования
        }

        SDL_RenderPresent(ren); // Обновляем экран для отображения всех нарисованных объектов
        // Задержка для плавного отображения и обработки событий на macOS
        SDL_Delay(10);
        SDL_Event windowEvent; // Событие для обработки взаимодействия с окном
        SDL_PollEvent(&windowEvent); // Проверка на наличие событий в окне
    }

    // Метод для вывода информации об ошибках в лог-файл
    void print_exception(const string& text) {
        ofstream fout(project_path + "log.txt", ios_base::app); // Открытие файла лога для записи
        fout << "Error: " << text << ". " << SDL_GetError() << endl; // Запись информации об ошибке
        fout.close(); // Закрытие файла
    }

public:
    int W = 0; // Ширина окна
    int H = 0; // Высота окна
    // История состояний доски для возможности отката ходов
    vector<vector<vector<POS_T>>> history_mtx;

private:
    SDL_Window* win = nullptr; // Указатель на окно SDL
    SDL_Renderer* ren = nullptr; // Указатель на рендерер SDL
    // Текстуры для изображений доски и фигур
    SDL_Texture* board = nullptr; // Текстура для доски
    SDL_Texture* w_piece = nullptr; // Текстура для белой фигуры
    SDL_Texture* b_piece = nullptr; // Текстура для черной фигуры
    SDL_Texture* w_queen = nullptr; // Текстура для белого ферзя
    SDL_Texture* b_queen = nullptr; // Текстура для черного ферзя
    SDL_Texture* back = nullptr; // Текстура для кнопки "назад"
    SDL_Texture* replay = nullptr; // Текстура для кнопки "повтор"

    // Пути к файлам текстур
    const string textures_path = project_path + "Textures/"; // Путь к папке с текстурами
    const string board_path = textures_path + "board.png"; // Путь к текстуре доски 

    const string piece_white_path = textures_path + "piece_white.png"; // Путь к текстуре белой фигуры
    const string piece_black_path = textures_path + "piece_black.png"; // Путь к текстуре черной фигуры
    const string queen_white_path = textures_path + "queen_white.png"; // Путь к текстуре белого ферзя
    const string queen_black_path = textures_path + "queen_black.png"; // Путь к текстуре черного ферзя
    const string white_path = textures_path + "white_wins.png"; // Путь к текстуре победы белых
    const string black_path = textures_path + "black_wins.png"; // Путь к текстуре победы черных
    const string draw_path = textures_path + "draw.png"; // Путь к текстуре ничьей
    const string back_path = textures_path + "back.png"; // Путь к текстуре для кнопки "назад"
    const string replay_path = textures_path + "replay.png"; // Путь к текстуре для кнопки "повтор"

    // Координаты выбранной активной клетки
    int active_x = -1, active_y = -1;
    // Результат игры (если он есть)
    int game_results = -1;
    // Матрица возможных ходов (подсветка клеток)
    vector<vector<bool>> is_highlighted_ = vector<vector<bool>>(8, vector<bool>(8, 0));

    // Матрица для представления состояния доски
    // 1 - белая фигура, 2 - черная фигура, 3 - белый ферзь, 4 - черный ферзь
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0));
    // Серия побитий для каждого хода
    vector<int> history_beat_series;
};