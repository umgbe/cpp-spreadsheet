#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    
    enum class CellType {
        EMPTY, TEXT, FORMULA
    };

    CellType type_ = CellType::EMPTY;

    std::string text_;

    std::unique_ptr<FormulaInterface> formula_;

    /*  Набор позиций, от которых зависит ячейка */
    std::unordered_set<Position> lower_cells_;
    /*  Набор позиций, которые зависят от ячейки */ 
    std::unordered_set<Position> upper_cells_;


    /*  Когда формула в ячейке меняется, следующими методами можно изменить
    lower_cells_ у себя и upper_cells_ у ячеек из формулы. */

    /*  Добавляет новый узел в список узлов, от которых зависит ячейка. */
    void AddLowerNode(Position cell);
    /*  Добавляет новый узел в список узлов, которые зависят от ячейки. */
    void AddUpperNode(Position cell);
    /*  Убирает узел из списка узлов, от которых зависит ячейка. */
    void RemoveLowerNode(Position cell);
    /*  Убирает узел из списка узлов, которые зависят от ячейки. */
    void RemoveUpperNode(Position cell);

    /*  Если ячейки, в которой нужно поменять upper_cells_ не существует, можно создать пустую */

   

    /*  Проверяет на наличие циклических зависимостей. 
    Если ниже нашёлся зацикленный узел, возвращается ссылка на узел. */
    std::optional<const Cell&> CheckNode() const;

    /*  Во время проверки каждый узел вызывает эту функцию для всех своих нижних
    узлов, дополняя набор своим узлом. Каждый нижний узел будет сравнивать себя
    со списком, спущенным сверху. Если найден узел с дубликатом, он поднимется наверх. */
    std::optional<const Cell&> CheckNode(std::unordered_set<const Cell*> cells);


    /*  При попытке изменения значения ячейки можно создать копию оригинальной
    ячейки и поместить её во временный буфер. На место старой ячейки ставится новая,
    затем происходят проверки на корректность таблицы. Если проверки пройдены, буфер уничтожается.
    Иначе, буфер возвращается в таблицу, и бросаются исключения. */

    Cell(Cell& other_cell); //Сюда можно отправить this
    void Swap(Cell& other_cell);


    /*  Кэш ячейки */
    std::optional<double> cache_;

    /*  Стирает кэш из ячейки и всех, которые зависят от неё */
    void InvalidateCache();

    Sheet& sheet_;

};
