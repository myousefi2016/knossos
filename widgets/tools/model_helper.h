#ifndef MODEL_HELPER
#define MODEL_HELPER

#include <QItemSelection>

template<typename Elem>
Elem & getElem(Elem & elem) {
    return elem;
}
template<typename Elem>
Elem & getElem(Elem * elem) {
    return *elem;
}
template<typename Elem>
Elem & getElem(const std::reference_wrapper<Elem> & elem) {
    return elem.get();
}

auto deltaBlockSelection = [](const auto & model, const auto & data, const auto isAlreadySelected){
    QItemSelection selectedItems;

    bool blockSelection{false};
    std::size_t blockStartIndex{0};

    std::size_t rowIndex{0};
    for (auto & elem : data) {
        const auto alreadySelected = isAlreadySelected(rowIndex);
        const auto selected = getElem(elem).selected;
        if (!blockSelection && selected && !alreadySelected) {// start block selection
            blockSelection = true;
            blockStartIndex = rowIndex;
        }
        if (blockSelection && (!selected || alreadySelected)) {// end block selection
            selectedItems.select(model.index(blockStartIndex, 0), model.index(rowIndex-1, model.columnCount()-1));
            blockSelection = false;
        }
        ++rowIndex;
    }
    // finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(model.index(blockStartIndex, 0), model.index(rowIndex-1, model.columnCount()-1));
    }

    return selectedItems;
};
auto blockSelection = [](const auto & model, const auto & data){
    return deltaBlockSelection(model, data, [](int){return false;});
};

auto threeWaySorting = [](auto & table, auto & sortIndex){// emulate ability for the user to disable sorting
    return [&table, &sortIndex](const int index){
        if (index == sortIndex && table.header()->sortIndicatorOrder() == Qt::SortOrder::AscendingOrder) {// asc (-1) → desc (==) → asc (==)
            table.sortByColumn(sortIndex = -1);
        } else {
            sortIndex = index;
        }
    };
};

#endif//MODEL_HELPER
