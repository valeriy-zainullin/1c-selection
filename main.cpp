#include <cassert>
#include <vector>
#include <filesystem>
#include <string_view>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <unordered_set>
#include <optional>
#include <span>

void print_usage(std::string_view program_path) {
    std::cout << "Usage: " << program_path <<  " [folder1] [folder2]\n";
}

namespace fs {
    using namespace std::filesystem;
};

void get_suffix_array(std::span<uint8_t> text, std::vector<size_t>& suffix_array, std::vector<size_t>& inv_suffix_array) {
    // Алгоритм Манбера-Майерса. Строим суффиксный массив за
    //   O(n log(n)) сортировкой зацикленных циклических сдвигов.
    // Допишем нулевой символ в конец, который меньше всех остальных.
    // Теперь суффикс из нулевого символа находится в начале массива.
    // Лексикографический порядок суффиксов не изменится (к ним
    //   просто дописался ноль):
    //   если не один из двух суффиксов не был префиксом другого,
    //   то в какой-то момент они начинают отличаться, до нуля
    //   сравнение не доходит;
    //   если был, то суффикс меньшей длины префикс суффикса большей,
    //   раньше сравнение завершалось из-за разной длины строк, но
    //   теперь у строки меньшей длины появляется нулевой символ
    //   раньше.
    // Теперь в конце суффиксов есть нулевой байт. Можем после этого
    //   байта дописать любые строки, порядок суффиксов не изменится:
    //   все суффиксы разной длины, потому там находится 0 в разных
    //   позициях. До дописанной строки сравнение никогда не дойдет,
    //   поскольку на первом нуле выяснится порядок, кто меньше. У
    //   кого встретился ноль, то и меньше.
    // Тогда допишем к суффиксам ноль, а потом недостающие символы
    //   из начала, а затем зациклим эту строку, сделав бесконечной.
    //   Достаточно отсортировать эти бесконечные строки по первым n
    //   символам, где n -- длина строки, и всё будет готово. Давайте
    //   отсортируем по первой степени двойки, которая больше или
    //   равна n. Сортировать будем сортировками подсчётом за O(n),
    //   количество сравниваемых строк -- n + 1, это O(n), будет
    //   log_2(n) итераций. Потому O(n log(n)).
    // Можно дописывать не нулевой символ, а строго меньший всех
    //   остальных в строке. Все доказательства будут работать.
    // Для нашей задачи можем дописать заведомо меньший символ
    //   0x10.
    // Пусть S_i -- номер отрезка совпадающих элементов i-го суффикса
    //   после сортировки с k-ой итерации.
    //   Как понять, что это такое? В результате любой сортировки
    //   у нас сначала идут числа с наименьшим значением, затем
    //   со вторым наибольшим и так далее. И числа с наименьшим,
    //   если их было несколько, образуют отрезок. Аналогично числа
    //   со вторым наименьшим значением и так далее.
    //   Это же происходит и при лексикографической сортировке строк:
    //     сначала сколько строк одинаковых, наименьших лексикографически,
    //     затем строки вторые лексикогарфически и так далее. Внутри
    //     отрезка находятся одинаковые строки, т.е. это просто одинаковые
    //     строки из набора. Пронумеруем эти отрезки с нуля, по индексу из
    //     исходного массива будем получать номер такого отрезка, где
    //     элемент оказался.
    //   В нашем случае мы сортируем по первым 2^k символам, потому по первым
    //     k символам строки совпадают. И сами строки не храним, а храним их
    //     начала в исходной строке.
    //   Это ещё называют лексикографическим именем (алгоритм
    //   Каркайнена-Сандерса, построение суфмассива за O(n)), компонентой
    //   эквивалентности в разборах нашего алгоритма, что имеет смысл, т.к.
    //    внутри отрезка все элементы равны.
    // Пусть M_j -- отсортированный массив с началами суффиксов с итерации k.
    // На итерации k + 1 надо отсортировать пары (S_i, S_{i + 2^k}).
    //   Отсортировать по второй половине легко: возьмём M'_j = M_{j + 2^k},
    //   зациклив индекс. Почему это то же самое, что отсортировать по второй
    //   половине? TODO: посмотреть разбор этого алгоса, дописать. Сейчас я
    //   TODO: просто знаю, что надо делать.
    // Чтобы выучить алгоритм, рекомендую записать эти шаги, держать их в
    //   голове.

    // Этапы:
    //  1) дополнение строки,
    //  1.1) почему дополняем, порядок суффиксов не меняется?
    //  1.2) почему ...
    //  1.2) почему ...
    //  1.2) почему ...
    //  1.3) почему можно сортировать по первым ceil(log_2(n)) символам и всё ок?
    //  2) сортировка по первому символа (2^k, k = 0),
    //  3) сортировка 2^(k + 1), если отсортировано по 2^k:
    //   3.1) сортировка по второй половине пары, почему действительно отсортровали.

    // Для 1.3 что-то такое можно использовать.
    // Сортируем зацикленные циклические сдвиги по первым 2^len_log символам.
    // После сортировать нет смысла, т.к. результаты сравнений не поменяются,
    //   потому что у всех зацикленных циклических суффиксов есть решетка,
    //   у различных по длине она в разных местах, потому знак
    //   неравенства становится гарантированно известен после просмотра
    //   первых text.size() символов: обязательно встретим решетку, она
    //   не совпадёт. Мб и раньше будет несовпадение.

    assert(!text.empty());

    // Этап 1: дополнение строки.
    std::vector<uint16_t> text_mod;
    text_mod.insert(text_mod.end(), text.begin(), text.end());
    text_mod.push_back(255 + 1);

    // Общие массивы для этапов, остаются с предыдущей итерации
    //   для новой.
    std::vector<size_t> sorted_items;
    std::vector<size_t> component_by_item(text_mod.size(), 0);

    // Этап 2.
    // Сортируем по первому символу, это первые 2^k
    //   символов для k = 0.
    // Для сортировок подсчётом на следующих итерациях
    //   надо знать алфавит, чтобы выделить массив.
    //   Наш алфавит -- номера компонент эквивалентности.
    //   На (k+1)-ой итерации мы сортируем по номерам с
    //   предыдущей итерации: TODO: ПРОВЕРИТЬ, КОГДА НАПИШУ КОД.
    //   Потому можно выделить массив на длину строки элементов,
    //   т.к. номер компоненты начинается с нуля и их не больше,
    //   чем суффиксов +1, это длина text_mod. Единственное, в
    //   чем беда: при сортировке подсчётом по первому символу
    //   номер компоненты равен номеру символа в алфавите.
    //   Пересчитаем номер компоненты заново после сортировки, что там.
    constexpr uint16_t kSrcStrAlphabetMaxChr = 255 + 1;
    // Сортируем строки длины 1.
    {
        std::vector<size_t> num_occurs(static_cast<size_t>(kSrcStrAlphabetMaxChr) + 1, 0);
        for (size_t i = 0; i < text_mod.size(); ++i) {
            const size_t digit = text_mod[i];
            num_occurs[digit] += 1;
        }
        // Исключающие префиксные суммы, как говорят публикации
        //   алгоритма Каркайнена-Сандерса.
        // Вообще, можно делать стабильную сортировку подсчётом
        //  двумя способами: считать исключающие суммы,
        //  количество элементов до компоненты, тогда перебирать
        //  в прямом порядке отсоритрованный массив, в каждую
        //  компоненту попадает наименьший. Или хранить включая,
        //  тогда перебирать в обратном, в каждую компоненту
        //  попадает наибольший, в конец компоненты.
        std::vector<size_t>& num_items_before_digit = num_occurs;
        size_t cur_digit_num_items_before = 0;
        for (size_t i = 0; i <= kSrcStrAlphabetMaxChr; ++i) {
            size_t num_occurences = num_occurs[i];
            num_items_before_digit[i] = cur_digit_num_items_before;
            // For the next iteration this pos is included.
            cur_digit_num_items_before += num_occurences;
        }
        sorted_items.assign(text_mod.size(), 0);
        for (size_t i = 0; i < text_mod.size(); ++i) {
            const size_t digit = text_mod[i];
            sorted_items[num_items_before_digit[digit]] = i;
            num_items_before_digit[digit] += 1;
        }
        // Считаем номера компонент эквивалентности.
        component_by_item[sorted_items[0]] = 0;
        for (size_t i = 1; i < sorted_items.size(); ++i) {
            // Пока это ещё символы, можно смотреть в текст.
            //   Дальше придется смотреть номера компонент
            //   эквивалентности. И тогда понадобится новый
            //   массив, чтобы не перезаписывать старые
            //   значения.
            // TODO: make this implementation detail, don't think about it.
            // TODO: make a small version of this algo, on associative containers,
            //   in python, so that it's small and concise, ready to be memorised.
            if (text[sorted_items[i]] != text[sorted_items[i - 1]]) {
                component_by_item[sorted_items[i]] = component_by_item[sorted_items[i - 1]] + 1;
            } else {
                component_by_item[sorted_items[i]] = component_by_item[sorted_items[i - 1]];
            }
        }
    }

    // TODO: write step text here. Read the original article, explain better.
    // Для перехода к следующей итерации нам нужна дополнительная память:
    //   новый порядок, потому что мы сортируем по второй половине
    //   сдвигом, чтобы воспользоваться в стабильной сортировке подсчётом.
    //   Но резуьтат сортировки подсчётом мы при этом процессе записываем
    //   в старый;
    //   и старые номера компонент по элементам, это наши цифры в сортироке
    //   подсчётом, т.к. мы после сортировки должны получить новые номера
    //   компонент по обоим парам, не только по первой, при этом мы
    //   используем массив старых компонент, чтобы сравнивать.
    // Мы не можем и проходится по старому, и писать туда новый порядок,
    //   т.к. это перезатрёт позиции, в которые мы ещё не зашли.
    // Если пишете какой-то алгоритм в первый раз, создавайте максимальное
    //   количество массивов, для каждой величины по смыслу: новые величины
    //   после итерации, старые до итерации и т.п. Потом в конце итерации
    //   замените старые на новые.
    std::vector<size_t> new_sorted_items(text_mod.size());
    std::vector<size_t> new_component_by_item(text_mod.size());
    // ull to avoid comparision between signed and unsigned, it's
    //   a warning. Don't think about it when you write it first
    //   time, you'll fix that.
    //   TODO: сделать секцию: особенности реализации на C++, там это указать.
    // While previous iteration was not enough.
    // (1ull << (len_log - 1)) < text_mod.size() is wrong. What'll happen?
    //   We need to sort by more than, length characters, This will stop
    //   the first time it's greater than length, without sorting!
    //   It may become greater, just only once! If it wasn't greater or
    //   equal before.
    for (size_t len_log = 1; (1ull << (len_log - 1)) < text_mod.size(); ++len_log) {
        // Сортируем по второй половине, просто сдвинув
        //   индексы: для каждой второй половины индекс
        //   первой половины определяется однозначно, а
        //   как отсортированы вторые половины знаем.
        const size_t len = static_cast<size_t>(1) << len_log;
        const size_t half_len = len / 2;
        // Отсортированы суффиксы по первым 2^(k - 1) символам.
        // Это вторые половины некоторых строк, т.к.
        //   строки зациклены.
        // Детально: просто возьмём любой суффикс, возьмём
        //   вторую половину из первых 2^k символов. Это тоже
        //   некоторый суффикс. И мы знаем порядок всех таких
        //   суффиксов, если оставим их и отсортируем
        //   (мультимножества строк совпадают).
        // Переидем к первым половинам, вычтя половину длины,
        //   получим упорядочивание строк длины 2^k по вторым
        //   половинам. Т.е. мы знаем строку с минимальной
        //   первой половиной, со второй по величине, если
        //   просто произведем операции. Тогда просто к каждой
        //   применим эту операцию и получим упорядочивание.
        // TODO: найти это в оригинальной публикации, посмотреть, как там пишут, обсудить.
        for (size_t i = 0; i < text_mod.size(); ++i) {
            new_sorted_items[i] = (sorted_items[i] + text_mod.size() - half_len) % text_mod.size();
        }
        // Сортировка по второй половине закончена.

        // Сортируем по первой половине (по компоненте первого элемента пары) подсчётом.
        // Сдвигов n, потому компонент эквивалентности нужно не более, чем n.
        // Чтобы сортировать устойчиво по второй половине, нам нужно проходить
        //   по элементам в перевернутом отсортированном порядке.
        //   Нам нужен отсортрованный порядок, потому будем его хранить.
        // TODO: потом вынести, чтобы постоянно память не выделять.
        //   Это в шаги по оптимизации написать. Код оставить, просто
        //   закомментировать. Указать наверху, что оптимизация №2, на первом
        //   написании пропустить. А здесь указать, первое написание, понятное.
        // Типичная сортировка подсчётом, правда алфавит -- компоненты
        //   эквивалентности с прошлого шага.
        {
            std::vector<size_t> num_occurs(text_mod.size(), 0);
            for (size_t i = 0; i < text_mod.size(); ++i) {
                const size_t digit = component_by_item[i];
                num_occurs[digit] += 1;
            }
            // Исключающие префиксные суммы, как говорят публикации
            //   алгоритма Каркайнена-Сандерса.
            std::vector<size_t>& num_items_before_digit = num_occurs;
            size_t cur_digit_num_items_before = 0;
            for (size_t i = 0; i < num_occurs.size(); ++i) {
                size_t num_occurences = num_occurs[i];
                num_items_before_digit[i] = cur_digit_num_items_before;
                // For the next iteration this pos is included.
                cur_digit_num_items_before += num_occurences;
            }
            // Перебираем пары в порядке сортировки по второй половине.
            for (size_t i: new_sorted_items) {
                // Берем компоненту первой половины.
                const size_t digit = component_by_item[i];
                sorted_items[num_items_before_digit[digit]] = i;
                num_items_before_digit[digit] += 1;
            }
            // Считаем номера компонент эквивалентности.
            new_component_by_item[sorted_items[0]] = 0;
            for (size_t i = 1; i < sorted_items.size(); ++i) {
                size_t prev_item = sorted_items[i - 1];
                size_t cur_item = sorted_items[i];

                size_t prev_item_first_half_comp  = component_by_item[prev_item];
                size_t prev_item_second_half_comp = component_by_item[(prev_item + half_len) % text_mod.size()];

                size_t cur_item_first_half_comp  = component_by_item[cur_item];
                size_t cur_item_second_half_comp = component_by_item[(cur_item + half_len) % text_mod.size()];

                // We already know (p_f, p_s) <= (c_f, c_s),
                //   it's p_f < c_f || (p_f == c_f && p_s <= c_s)
                // We just they are not equal we just need to
                //   check p_f < c_f || p_s < c_s, because if
                //   p_f < c_f is not true, p_f >= c_f,
                //   then p_f == c_f.
                if (prev_item_first_half_comp < cur_item_first_half_comp || prev_item_second_half_comp < cur_item_second_half_comp) {
                    new_component_by_item[cur_item] = new_component_by_item[prev_item] + 1;
                } else {
                    new_component_by_item[cur_item] = new_component_by_item[prev_item];
                }
            }
            // Массив новых компонент переходит в следующую итерацию,
            //   а предыдущий используем как временную память.
            //   Мы её перезапишем.
            // А массив отсортированного порядка мы поменяли ещё
            //   при сортировке по первой половине.
            std::swap(new_component_by_item, component_by_item);
        }
    }
    // После всех итераций отсортировали по большому количеству символов (>= n),
    //   по факту получили сортировку суффиксов.
    // Только удалим символ ноль, он лежит первым, т.к. это самый маленький
    //   суффикс.
    //   TODO: добавить в шаги, указать здесь шаг, как выше.
    sorted_items.erase(sorted_items.begin(), sorted_items.begin() + 1);
    suffix_array = std::move(sorted_items);
    // После всех итераций размер каждой компоненты равен одному,
    //   т.к. все элементы различны. И это просто индекс суффикса
    //   в суффиксном массиве (обратный суффиксный массив).
    //   TODO: добавить в шаги, указать здесь шаг, как выше.
    //   TODO: указать, зачем обратный суффиксный массив и в шаге, и тут: для lcp.
    // Только нужно удалить компоненту по item-у text.size(), т.к это нулевой символ.
    //   И все компоненты сдвинуть на один, т.к. первая компонента -- компонента
    //   нулевого символа.
    component_by_item.erase(component_by_item.end() - 1, component_by_item.end());
    for (size_t& component: component_by_item) {
        component -= 1;
    }
    inv_suffix_array = std::move(component_by_item);
}

std::vector<size_t> calculate_lcp(const std::span<uint8_t> text, const std::vector<size_t>& suffix_array, const std::vector<size_t>& inv_suffix_array) {
    assert(!text.empty());

    // Алгоритм Аримуры-Арикавы-Касаи-Ли-Парка
    //   построение массива lcp для суффиксного
    //   массива.
    // Пусть sa -- суффиксный массив,
    //   isa    -- обратный суффиксный массив
    //     (по номеру суффикса позиция в
    //      суффиксном массиве),
    //   text   -- текст.
    // Основное утверждение:
    //   lcp[isa[i + 1]] >= lcp[isa[i]] - 1.
    //   Возьмём суффикс, у которого есть следующий в суффиксном массиве.
    //   У него есть какой-то lcp с предыдущим суффиксом в суффиксном массиве.
    //   Если lcp с предыдущим символом равен нулю, то получаем неравенство
    //   lcp[*] >= -1, что всегда верно, ведь lcp не отрицателен.
    //   Если lcp с предыдущим символом как минимум один, то удалим их общий
    //   первый символ. Получим два суффикса, которые так же упорядочены:
    //   предыдущий суффикс был меньше текущего, т.е. в первой несовпадающей
    //   позиции он меньше, или совпадает во всех, но длина меньше (суффиксы
    //   с разным началом никогда не равны, т.к. у них как минимум разные
    //   длины, если даже все символы совпадают). Тогда после удаления
    //   первого совпадающего символа сравнение такое же. Наидлиннейший
    //   отрезок совпадающих первых символов сдвинулся на один символ, его
    //   длина уменьшилась на один символ. Значит, lcp между этими двумя
    //   отрезками ровно lcp[isa[i]] - 1. Но они могли быть не соседними:
    //   просто предположим, что не соседние, тогда допишем символ, к этим
    //   двум суффиксам можем дописать, а возможно, что к суффиксу между
    //   ними не можем дописать этот символ, там другой. Не смогли доказать,
    //   и реально такой пример может быть. Потому просто есть неравенство,
    //   что сколько-то длины lcp у нас уже есть.
    // Лучше объяснено тут: https://www.youtube.com/watch?v=s68lnO3yEKY&list=PLtb_PNVHdsV7QlpdH1XmsqqF9AxYRQpWY&index=19
    // А теперь как сделать алгоритм? Будем двигаться от самых длинных
    //   суффиксов к коротким (т.е. просто от начала строки к концу,
    //   перебирать начало суффикса в строке). Для коротких у нас уже
    //   будет неравенство от их продолжений, что сколько-то lcp у них
    //   есть, это похоже на z-функцию. Можно попробовать увеличить.
    //   Переход к следующему суффиксу и есть удаление первого символа,
    //   потому lcp -- просто переменная между итерациями.
    // За сколько это работает? Каждую итерацию убывает на один,
    //   увеличивается на много, но сама величина 0 <= lcp <= |t|.
    // Амортизационный анализ. Итерации связаны с изменением некоторой
    //   величины, на которую есть оценка. Пусть n_i -- количество
    //   итераций while с попыткой увеличить lcp.
    //   TODO: написать, мб к экзамену. Кажется, сейчас уже нет смысла,
    //   стало понятно. n_i = lcp_i - lcp_{i - 1}...
    //   sum (C1 + C2 n_i) <= C_1 * n + C_2 * (lcp_{n} - lcp_0) <= C n
    // Только тут ещё есть проблема, как мы обходим случай, когда
    //   встречаем суффикс, который первый в суффиксном массиве? Тогда
    //   либо на предыдущей итерации был lcp = 0, тогда мы просто
    //   наивно посчитаем, всё ок так же по амортизированному анализу.
    //   Либо предыдущий в СА суффикс на предыдущей итерации был из
    //   одного символа. Иначе lcp > 0, у нас был предыдущий суффикс
    //   в СА, а после удаления символа нет, тогда получился пустой
    //   суффикс просто там, если там длина хотя бы два, то суффикс
    //   меньше будет. lcp = 1 на прошлой итерации, на этой оценка
    //   снизу ноль. Посчитаем явно на следующей, всё ок.

    std::vector<size_t> result(text.size() - 1, 0);
    size_t lcp_lower_bound = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        size_t sa_pos = inv_suffix_array[i];
        if (sa_pos == 0) {
            continue;
        }

        size_t sa_prev_suffix = suffix_array[sa_pos - 1];

        size_t lcp = lcp_lower_bound;
        size_t cur_suffix_len = text.size() - i;
        size_t sa_prev_suffix_len = text.size() - sa_prev_suffix;
        while (lcp < cur_suffix_len && lcp < sa_prev_suffix_len && text[sa_prev_suffix + lcp] == text[i + lcp]) {
            lcp += 1;
        }

        // В массиве lcp индекс -- индекс суффикса в суффиксном массиве,
        //   значение -- длина наидлиннейшего общий префикса со
        //   следующим, а мы для каждой пары соседних в суффиксном
        //   массиве перебирали второго из пары, потому надо уменьшить.
        // То же самое получается, если в lcp хранить длину наидлиннейшего
        //   общий префикса с предыдущим.
        result[sa_pos - 1] = lcp;

        // Без нижней оценки на lcp, т.е. без того утверждения (леммы Касаи)
        //   если выполнять, количество итераций while наверху не соответстует
        //   изменению lcp, поскольку мы заново начинаем с 0, а с оценкой снизу
        //   начинаем с почти предыдущего значения.
        if (lcp == 0) {
            lcp_lower_bound = 0;
        } else {
            lcp_lower_bound = lcp - 1;
        }
    }

    return result;
}

template<typename T>
std::ostream& operator<<(std::ostream& stream, const std::span<T>& span) {
    std::cout << '{';
    for (auto& item: span) {
        std::cout << item;
        std::cout << ',';
    }
    std::cout << '}';
    return stream;
}

template<typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& vector) {
    stream << std::span(vector.begin(), vector.end());
    return stream;
}

size_t get_longest_cmn_substr_len(const std::span<uint8_t>& first, const std::span<uint8_t>& second) {
    std::vector<uint8_t> joined_vector;
    joined_vector.insert(joined_vector.end(), first.begin(), first.end());
    joined_vector.insert(joined_vector.end(), second.begin(), second.end());

    // std::cout << "joined_vector = " << joined_vector << '\n';

    std::vector<size_t> suffix_array;
    std::vector<size_t> inv_suffix_array;
    get_suffix_array(joined_vector, suffix_array, inv_suffix_array);
    std::vector<size_t> lcp = calculate_lcp(joined_vector, suffix_array, inv_suffix_array);

    // TODO: Проверить, что в начале нет пустого суффиекса.

    // Суффиксы совпадающих подстрок наибольшей длины находятся рядом в суффиксном массиве.
    size_t result = 0;
    for (size_t i = 1; i < suffix_array.size(); ++i) {
        bool prev_is_from_first = suffix_array[i - 1] < first.size();
        bool cur_is_from_first  = suffix_array[i] < first.size();
        if (prev_is_from_first != cur_is_from_first) {
            result = std::max(result, lcp[i - 1]);
        }
    }

    return result;
}

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
		return 1;
	}

	// В ответе требуется предоставить
	//   результаты для каждой пары файлов
	//   их похожесть. Пусть в первой директории
	//   n файлов, а во второй -- m. a_i -- длина
	//   i-го файла в первой директории, b_j --
	//   длина j-го файла во второй директории.
	// Мы можем построить суффиксное дерево
	//   для файла из первой директории, а ходить
	//   по нему, как по бору (потому что это бор)
	//   из алгоритма Ахо-Корасик.
	// Наша задача -- для каждой пары файлов
	//   найти наидлиннейшую общую подстроку
	//   для каждых двух файлов.
	// Алгоритм: составляем список путей к файлам,
	//   два вектора с именами файлов директорий,
	//   перебираем файл из первой директории,
    //   перебираем файл из второй директории,
    //   решаем задачу наидлиннейшей общей подстроки
    //   из алгоритмов с помощью суффиксного массива
    //   и lcp.
    // За сколько работает? sum_{i от 1 до n} sum_{j от 1 до m}
    //   O((n_i + m_j) log(n_i + m_j)).
    // За сколько памяти? Требуется max_{i, j} (n_i + m_j) * C,
    //   C не более 8 + 8 + 8 + 2 байт. При сравнений файлов в 10
    //   мегабайт, потребуется 520 мегабайт. Можем себе позволить.

	std::string_view dir1(argv[1]);
    std::string_view dir2(argv[2]);

    int percent_for_not_eq = 100;
    if (argc == 4) {
        auto percent_sv = std::string_view(argv[3]);
        for (char chr: percent_sv) {
            if (chr < '0' || chr > '9') {
                return 2;
            }
        }
        // TODO: check value didn't overflow, fits into int.
        size_t num_chrs_processed = 0;
        percent_for_not_eq = std::stoi(std::string(percent_sv), &num_chrs_processed);
        if (num_chrs_processed != percent_sv.size() || percent_for_not_eq < 0 || percent_for_not_eq > 100) {
            std::cout << "Invalid percentage value." << '\n';
        }
    }


    std::unordered_set<std::string> dir1_items_not_in_dir2;
    std::unordered_set<std::string> dir2_items_in_dir1;

	for (const auto& item_dir1: fs::directory_iterator(dir1)) {
        std::vector<uint8_t> item_dir1_content;
        {
            std::ifstream stream(item_dir1.path());
            uint8_t byte = 0;
            while (true) {
                * reinterpret_cast<char*>(&byte) = stream.get();
                if (stream.fail()) {
                    break;
                }
                item_dir1_content.push_back(byte);
            }
        }

        size_t item_dir1_size = reinterpret_cast<size_t>(item_dir1.file_size());
        bool found_in_dir2 = false;
		for (const auto& item_dir2: fs::directory_iterator(dir2)) {
            std::vector<uint8_t> item_dir2_content;
            {
                std::ifstream stream(item_dir2.path());
                uint8_t byte = 0;
                while (true) {
                    * reinterpret_cast<char*>(&byte) = stream.get();
                    if (stream.fail()) {
                        break;
                    }
                    item_dir2_content.push_back(byte);
                }
            }

            size_t cmn_substr_size = get_longest_cmn_substr_len(item_dir1_content, item_dir2_content);

            size_t max_size = std::max(item_dir1_content.size(), item_dir2_content.size());
            // std::cout << "cmn_substr_size = " << cmn_substr_size << std::endl;
            std::cout << dir1 << "/" << item_dir1.path().filename().string() << " - " << dir2 << "/" << item_dir2.path().filename().string();
            // cmn_substr_size * 100 / max_len < percent_for_not_eq
            //  Если процент больше, то файлы считаются одинаковыми.
            //  Избегаем округлений, работая с целыми числами.
            if (cmn_substr_size * 100 >= max_size * percent_for_not_eq) {
                std::cout << '\n';
                dir2_items_in_dir1.insert(fs::absolute(item_dir2.path()).string());
                found_in_dir2 = true;
            } else {
                std::cout << " - " << std::fixed << std::setw(0) << (cmn_substr_size * 100 / max_size) << '\n';
            }

            // The process may take long, show the user that there is
            //   some progress.
            std::cout.flush();
		}
        if (!found_in_dir2) {
            dir1_items_not_in_dir2.insert(fs::absolute(item_dir1.path()).string());
        }
	}

    for (const auto& item_dir1: dir1_items_not_in_dir2) {
        std::cout << dir1 << '/' << fs::path(item_dir1).filename().string() << ";";
    }
    std::cout << '\n';

    for (const auto& item_dir2: fs::directory_iterator(dir2)) {
        auto abs_path = fs::absolute(item_dir2).string();
        if (dir2_items_in_dir1.count(abs_path) > 0) {
            continue;
        }
        std::cout << dir2 << '/' << item_dir2.path().filename().string() << ";";
    }
    std::cout << '\n';

    return 0;
}
