//  Copyright (c) 2016 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#include <toyui/Config.h>
#include <toyui/Widget/Sheet.h>

#include <toyui/Frame/Frame.h>

#include <toyui/Input/InputDevice.h>

namespace toy
{
	WidgetStore::WidgetStore(Wedge& wedge)
		: m_wedge(wedge)
	{}

	Widget& WidgetStore::append(object_ptr<Widget> pointer)
	{
		Widget& widget = *pointer;
		m_contents.push_back(std::move(pointer));
		widget.m_container = &m_wedge;
		return widget;
	}

	void WidgetStore::remove(Widget& widget)
	{
		m_wedge.remove(widget);
		vector_remove_pt(m_contents, widget);
	}

	void WidgetStore::transfer(Widget& widget, Wedge& target)
	{
		m_wedge.remove(widget);
		target.append(widget);
		widget.m_container = &target;
		vector_transfer_pt(m_contents, target.store().m_contents, widget);
	}

	void WidgetStore::clear()
	{
		for(auto& widget : reverse_adapt(m_contents))
		{
			widget->destroyTree();
			m_wedge.remove(*widget);
		}

		m_contents.clear();
	}

	Wedge::Wedge(const Params& params)
		: Widget({ params, &cls<Wedge>() })
	{}

	void Wedge::visit(const Visitor& visitor)
	{
		bool visit = true;
		visitor(*this, visit);

		if(visit)
			for(Widget* pwidget : m_contents)
				pwidget->visit(visitor);
	}

	void Wedge::reindex(size_t from)
	{
		for(size_t i = from; i < m_contents.size(); ++i)
			m_contents[i]->m_index = i;
		m_frame->markDirty(DIRTY_STRUCTURE);
	}

	void Wedge::append(Widget& widget)
	{
		m_contents.push_back(&widget);
		widget.bind(*this, m_contents.size() - 1);
	}

	void Wedge::insert(Widget& widget, size_t index)
	{
		m_contents.insert(m_contents.begin() + index, &widget);
		widget.bind(*this, index);
		this->reindex(index);
	}

	void Wedge::remove(Widget& widget)
	{
		m_contents.erase(m_contents.begin() + widget.m_index);
		this->reindex(widget.m_index);
		widget.unbind();
	}

	void Wedge::move(size_t from, size_t to)
	{
		m_contents.insert(m_contents.begin() + to, m_contents[from]);
		m_contents.erase(m_contents.begin() + from + 1);
		this->reindex(from < to ? from : to);
	}

	void Wedge::swap(size_t from, size_t to)
	{
		std::iter_swap(m_contents.begin() + from, m_contents.begin() + to);
		this->reindex(from < to ? from : to);
	}

	GridSheet::GridSheet(const Params& params, Dimension dim, Callback callback)
		: Wedge({ params, &cls<GridSheet>() })
		, m_dim(dim)
		, m_dragPrev(nullptr)
		, m_dragNext(nullptr)
		, m_onResize(callback)
	{}

	bool GridSheet::leftDragStart(MouseEvent& mouseEvent)
	{
		// we take the position BEFORE the mouse moved as a reference
		
		DimFloat local = m_frame->localPosition(mouseEvent.m_pressed);
		m_dragPrev = nullptr;
		m_dragNext = nullptr;

		for(auto& widget : this->m_contents)
		{
			if(widget->frame().d_position[m_dim] >= local[m_dim])
			{
				m_dragNext = &widget->frame();
				break;
			}
			m_dragPrev = &widget->frame();
		}

		return true;
	}

	bool GridSheet::leftDrag(MouseEvent& mouseEvent)
	{
		if(!m_dragNext || !m_dragPrev)
			return true;

		m_frame->transferPixelSpan(*m_dragPrev, *m_dragNext, m_dim, mouseEvent.m_delta[m_dim]);
		if(m_onResize)
			m_onResize(*m_dragPrev, *m_dragNext);
		return true;
	}
}
