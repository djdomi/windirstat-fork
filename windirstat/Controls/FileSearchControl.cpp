// WinDirStat - Directory Statistics
// Copyright © WinDirStat Team
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "pch.h"
#include "ItemSearch.h"
#include "FileTreeView.h"

CFileSearchControl::CFileSearchControl() : CTreeListControl(COptions::SearchViewColumnOrder.Ptr(), COptions::SearchViewColumnWidths.Ptr(), COptions::SearchViewColumnVisibility.Ptr(), LF_SEARCHLIST, false)
{
    m_singleton = this;
}

bool CFileSearchControl::GetAscendingDefault(const int column)
{
    return column == COL_ITEMSEARCH_NAME || column == COL_ITEMSEARCH_LAST_CHANGE;
}

BEGIN_MESSAGE_MAP(CFileSearchControl, CTreeListControl)
END_MESSAGE_MAP()

std::wregex CFileSearchControl::ComputeSearchRegex(const std::wstring & searchTerm, const bool searchCase, const bool useRegex)
{
    try
    {
        // Validate input is valid
        if (searchTerm.empty()) return {};

        // Decode regex flags based on settings
        auto searchFlags = std::regex_constants::optimize;
        if (!searchCase) searchFlags |= std::regex_constants::icase;

        // Precompile regex string
        return std::wregex(useRegex ?
            searchTerm : GlobToRegex(searchTerm, false), searchFlags);
    }
    catch (...)
    {
        return {};
    }
}

void CFileSearchControl::ProcessSearch(CItem* item,
    const std::wstring & searchTerm, const bool searchCase,
    const bool searchWholePhrase, const bool searchRegex, const bool onlyFiles)
{
    // Update tab visibility to show search tab if results exist
    CMainFrame::Get()->GetFileTabbedView()->SetSearchTabVisibility(true);

    // Process search request using progress dialog
    std::vector<CItem*> matchedItems;
    CProgressDlg(static_cast<size_t>(item->GetItemsCount()), CProgressDlg::Flags::None, AfxGetMainWnd(),
        [&](CProgressDlg* pdlg)
    {
        // Remove previous results
        SetRootItem();
        m_rootItem->SetLimitExceeded(false);

        // Precompile regex string
        const auto searchTermRegex = ComputeSearchRegex(searchTerm,
            searchCase, searchRegex);

        // Do search
        std::vector<CItem*> queue{ item };
        while (!queue.empty() && !pdlg->IsCancelled())
        {
            // Grab item from queue
            pdlg->Increment();
            CItem* qitem = queue.back();
            queue.pop_back();

            // Check for match
            if (!onlyFiles || qitem->IsTypeOrFlag(IT_FILE))
            {
                const auto nameView = qitem->GetNameView();
                const bool isMatch = searchWholePhrase ?
                    std::regex_match(nameView.begin(), nameView.end(), searchTermRegex) :
                    std::regex_search(nameView.begin(), nameView.end(), searchTermRegex);

                if (isMatch)
                {
                    matchedItems.push_back(qitem);
                }
            }

            // Descend into child items
            if (qitem->IsLeaf() || qitem->IsTypeOrFlag(IT_HLINKS)) continue;
            for (const auto& child : qitem->GetChildren())
            {
                queue.push_back(child);
            }
        }

        // Sort by physical size (largest first) and take top N results
        const size_t maxResults = COptions::SearchMaxResults;
        if (matchedItems.size() > maxResults)
        {
            // Partial sort to get the top N items by physical size
            std::ranges::partial_sort(matchedItems, matchedItems.begin() + maxResults,
                std::ranges::greater{}, &CItem::GetSizeLogical);

            // Keep only the top N results
            matchedItems.resize(maxResults);
            m_rootItem->SetLimitExceeded(true);
        }
    }).DoModal();

    PopulateSearchResults(matchedItems);
}

void CFileSearchControl::PopulateSearchResults(const std::vector<CItem*>& matchedItems)
{
    // Add found items to the interface
    CWaitCursor wait;
    CollapseItem(0);

    // Add to found items, each as a direct child of the results root
    const CSetRedrawLock lock(this);
    m_itemTracker.reserve(matchedItems.size());
    for (CItem* matchedItem : matchedItems)
    {
        auto searchItem = new CItemSearch(matchedItem);
        m_itemTracker.emplace(matchedItem, searchItem);
        m_rootItem->AddSearchItemChild(searchItem);
    }

    SortItems();
    ExpandItem(0);
}

void CFileSearchControl::PopulateSearchResultsHierarchical(const std::vector<CItem*>& matchedItems)
{
    // Add found items to the interface, nested under their real filesystem parent when
    // that parent is also part of the result set - so a match contained inside another
    // match is shown as its child instead of as a separate, seemingly-independent entry.
    CWaitCursor wait;
    CollapseItem(0);

    const CSetRedrawLock lock(this);
    m_itemTracker.reserve(matchedItems.size());
    for (CItem* matchedItem : matchedItems)
    {
        auto searchItem = new CItemSearch(matchedItem);
        m_itemTracker.emplace(matchedItem, searchItem);

        const auto parentIt = m_itemTracker.find(matchedItem->GetParent());
        CItemSearch* parentNode = (matchedItem->GetParent() != nullptr && parentIt != m_itemTracker.end())
            ? parentIt->second : m_rootItem;
        parentNode->AddSearchItemChild(searchItem);
    }

    SortItems();
    ExpandItem(0);
}

void CFileSearchControl::RemoveItem(CItem* item)
{
    const CSetRedrawLock lock(this);

    // Gather every tracker entry affected by this deletion before touching any node.
    // Deleting a node recursively destroys its own descendant nodes too (~CItemSearch),
    // so a nested match's CItemSearch pointer would already be dangling by the time it's
    // reached if entries were detached and erased in a single pass in arbitrary map order.
    std::vector<std::pair<CItem*, CItemSearch*>> matches;
    for (const auto& pair : m_itemTracker)
    {
        if (pair.first == item || item->IsAncestorOf(pair.first)) matches.emplace_back(pair);
    }

    // Detach only the topmost matched node per branch - RemoveSearchItemChild's destructor
    // cascade takes care of any matched descendants nested inside it. "Nested inside" has
    // to mean nested in this search-result tree, not just a filesystem descendant: flat
    // (non-hierarchical) results are always direct children of m_rootItem regardless of
    // real filesystem relationship, so a plain CItem::IsAncestorOf check here would wrongly
    // skip detaching a flat sibling match, leaving it visible but no longer tracked - and
    // pointing at a CItem that this same deletion is about to destroy.
    for (const auto& [matchedItem, node] : matches)
    {
        const bool hasMatchedAncestor = std::ranges::any_of(matches, [&](const auto& other)
        {
            if (other.second == node) return false;
            for (auto* p = static_cast<CItemSearch*>(node->GetParent()); p != nullptr;
                p = static_cast<CItemSearch*>(p->GetParent()))
            {
                if (p == other.second) return true;
            }
            return false;
        });
        if (hasMatchedAncestor) continue;

        auto* parent = static_cast<CItemSearch*>(node->GetParent());
        (parent != nullptr ? parent : m_rootItem)->RemoveSearchItemChild(node);
    }

    std::erase_if(m_itemTracker, [&](const auto& pair)
    {
        return pair.first == item || item->IsAncestorOf(pair.first);
    });
}

void CFileSearchControl::AfterDeleteAllItems()
{
    // Delete previous search results
    m_itemTracker.clear();

    // Delete and recreate root item
    delete m_rootItem;
    m_rootItem = new CItemSearch();
    InsertItem(0, m_rootItem);
    m_rootItem->SetExpanded(true);
}
