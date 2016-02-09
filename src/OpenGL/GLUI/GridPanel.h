#pragma once

#include "GLUI.h"
#include "Panel.h"

namespace GLUI
{
	/*class LayoutManager
	{
	public:
		LayoutManager(padding_t padding = padding_t()) : padding(padding) {}
		virtual ~LayoutManager() {}

		padding_t	getPadding() { return padding; }
		void		setPadding(padding_t padding) { this->padding = padding; }

		virtual void	updateLayout() {}
		virtual void	applyLayout(dim2_t fit) {}
		virtual dim2_t	getMinimumSize() { return dim2_t(0, 0); }
		virtual dim2_t	getPreferredSize(dim2_t proposed) { return dim2_t(0, 0); }

	protected:
		padding_t	padding;
	};*/

	class GridPos
	{
	public:
		GridPos(unsigned row, unsigned column, unsigned span_rows = 1, unsigned span_columns = 1,
			unsigned anchor_h = ANCHOR_CENTER, unsigned anchor_v = ANCHOR_CENTER)
			: widget(NULL), row(row), column(column), span_rows(span_rows), span_columns(span_columns),
			anchor_h(anchor_h), anchor_v(anchor_v) {}
		~GridPos() {}

		Widget*		getWidget() { return widget; }
		unsigned	getColumn() { return column; }
		unsigned	getRow() { return row; }
		unsigned	getColumnSpan() { return span_columns; }
		unsigned	getRowSpan() { return span_rows; }
		unsigned	getVerticalAnchor() { return anchor_v; }
		unsigned	getHorizontalAnchor() { return anchor_h; }

		void	setWidget(Widget* widget) { this->widget = widget; }

		enum
		{
			ANCHOR_LEFT = 0,
			ANCHOR_RIGHT = 1,
			ANCHOR_CENTER = 2,
			ANCHOR_TOP = 0,
			ANCHOR_BOTTOM = 1,
			ANCHOR_FILL
		};

	private:
		Widget*		widget;
		unsigned	column;
		unsigned	row;
		unsigned	span_columns;
		unsigned	span_rows;
		unsigned	anchor_v;
		unsigned	anchor_h;
	};

	class GridPanel : public Panel
	{
	public:
		struct column_t
		{
			unsigned	size;
			unsigned	min_size;
			unsigned	position;
			column_t()
			{
				size = 0;
				min_size = 0;
				position = 0;
			}
		};
		typedef column_t row_t;

		GridPanel(Widget* parent, padding_t padding = padding_t(0), dim2_t spacing = dim2_t(0, 0));
		~GridPanel();

		void	setColumnStretch(unsigned column, bool stretch = true);
		void	setRowStretch(unsigned row, bool stretch = true);

		void	addWidget(Widget* widget, GridPos position);
		void	calculateColumns(int max_width);
		void	calculateRows(int max_height);
		void	updateLayout();
		void	applyLayout(dim2_t fit);

		// Testing
		void	drawGrid(point2_t pos);

	private:
		vector<GridPos>		grid;
		vector<column_t>	columns;
		vector<unsigned>	stretch_columns;
		vector<row_t>		rows;
		vector<unsigned>	stretch_rows;

		dim2_t		spacing;
		padding_t	padding;
	};
}
