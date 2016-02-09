
#include "Main.h"
#include "GridPanel.h"
#include "Widget.h"

// Testing
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"

using namespace GLUI;


GridPanel::GridPanel(Widget* parent, padding_t padding, dim2_t spacing)
	: Panel(parent), padding(padding), spacing(spacing)
{
}

GridPanel::~GridPanel()
{
}

void GridPanel::setColumnStretch(unsigned column, bool stretch)
{
	if (stretch)
		VECTOR_ADD_UNIQUE(stretch_columns, column);
	else
		VECTOR_REMOVE(stretch_columns, column);
}

void GridPanel::setRowStretch(unsigned row, bool stretch)
{
	if (stretch)
		VECTOR_ADD_UNIQUE(stretch_rows, row);
	else
		VECTOR_REMOVE(stretch_rows, row);
}

void GridPanel::addWidget(Widget* widget, GridPos position)
{
	// Remove widget from the grid if it is there already
	for (int a = 0; a < (int)grid.size(); a++)
	{
		if (grid[a].getWidget() == widget)
		{
			grid.erase(grid.begin() + a);
			a--;
		}
	}

	// Add to grid
	grid.push_back(position);
	grid.back().setWidget(widget);
}

void GridPanel::calculateColumns(int max_width)
{
	/*
	// (Re)build column list to fit all widgets
	unsigned ncols = 0;
	for (unsigned a = 0; a < grid.size(); a++)
	{
		int max = grid[a].getColumn() + grid[a].getColumnSpan();
		if (max > ncols)
			ncols = max;
	}
	columns.resize(ncols);

	// Adjust for spacing & padding
	int total_spacing = spacing.x * (columns.size() - 1);
	total_spacing += padding.horizontal();
	if (max_width > total_spacing)
		max_width -= total_spacing;

	// First up, set all columns to preferred widget sizes,
	// and set minimum column sizes
	for (unsigned a = 0; a < grid.size(); a++)
	{
		Widget* widget = grid[a].getWidget();

		if (!widget->isVisible())
			continue;

		int pref_width = widget->getPreferredSize().x;
		int min_width = widget->getMinimumSize().x;

		if (grid[a].getColumnSpan() <= 1)
		{
			if (pref_width > columns[grid[a].getColumn()].size)
				columns[grid[a].getColumn()].size = pref_width;
			if (min_width > columns[grid[a].getColumn()].min_size)
				columns[grid[a].getColumn()].min_size = min_width;
		}
	}

	// Now process multi-column-spanned widgets
	for (unsigned a = 0; a < grid.size(); a++)
	{
		Widget* widget = grid[a].getWidget();

		if (grid[a].getColumnSpan() <= 1 || !widget->isVisible())
			continue;

		int pref_width = widget->getPreferredSize().x;
		int min_width = widget->getMinimumSize().x;

		// Get total widths (pref/min) of spanned columns
		int total_size = 0;
		int total_min_size = 0;
		for (unsigned c = grid[a].getColumn(); c < grid[a].getColumn() + grid[a].getColumnSpan(); c++)
		{
			total_size += columns[c].size;
			total_min_size += columns[c].min_size;
		}

		// Increase size of last spanned column if the total preferred size is too small
		if (total_size < pref_width)
			columns[grid[a].getColumn() + grid[a].getColumnSpan() - 1].size += (pref_width - total_size);

		// Increase minimum size of last spanned column if the total min size is too small
		if (total_min_size < min_width)
			columns[grid[a].getColumn() + grid[a].getColumnSpan() - 1].min_size += (min_width - total_min_size);
	}

	// Get the total width
	int total_width = 0;
	for (unsigned a = 0; a < columns.size(); a++)
		total_width += columns[a].size;

	// Check if we need to squish columns
	if (max_width >= 0 && total_width > max_width)
	{
		// Squish stretchable columns to min size first
		for (unsigned a = 0; a < stretch_columns.size(); a++)
		{
			int diff = columns[stretch_columns[a]].size - columns[stretch_columns[a]].min_size;
			columns[stretch_columns[a]].size = columns[stretch_columns[a]].min_size;
			total_width -= diff;

			if (total_width < max_width)
			{
				// Fit to max width
				columns[stretch_columns[a]].size += (max_width - total_width);
				break;
			}
		}

		// If squishing stretchable columns wasn't enough,
		// squish regular columns to min size starting from the right,
		// until we get below the max width (or not, in which
		// case it'll just have to be cut off)
		for (int a = columns.size() - 1; a >= 0; a--)
		{
			int diff = columns[a].size - columns[a].min_size;
			columns[a].size = columns[a].min_size;
			total_width -= diff;

			if (total_width < max_width)
			{
				// Fit to max width
				columns[a].size += (max_width - total_width);
				break;
			}
		}
	}
	else if (max_width >= 0)
	{
		// We have width to spare, so evenly distribute it between stretchable columns
		int extra = max_width - total_width;

		if (!stretch_columns.empty())
		{
			unsigned adjust = extra / stretch_columns.size();
			for (unsigned a = 0; a < stretch_columns.size(); a++)
				columns[stretch_columns[a]].size += adjust;

			// Add rounding remainder to last stretchable column
			if (adjust * stretch_columns.size() < extra)
				columns[stretch_columns.back()].size += (extra - (adjust * stretch_columns.size()));
		}
	}

	// Finally, calculate column positions
	unsigned pos = padding.left;
	for (unsigned a = 0; a < columns.size(); a++)
	{
		columns[a].position = pos;
		pos += columns[a].size + spacing.x;
	}
	*/
}

void GridPanel::calculateRows(int max_height)
{
	/*
	// (Re)build row list to fit all widgets
	unsigned nrows = 0;
	for (unsigned a = 0; a < grid.size(); a++)
	{
		int max = grid[a].getRow() + grid[a].getRowSpan();
		if (max > nrows)
			nrows = max;
	}
	rows.resize(nrows);

	// Adjust for spacing
	int total_spacing = spacing.y * (rows.size() - 1);
	total_spacing += padding.vertical();
	if (max_height > total_spacing)
		max_height -= total_spacing;

	// First up, set all rows to preferred widget sizes,
	// and set minimum row sizes
	for (unsigned a = 0; a < grid.size(); a++)
	{
		Widget* widget = grid[a].getWidget();

		if (!widget->isVisible())
			continue;

		int pref_height = widget->getPreferredSize().y;
		int min_height = widget->getMinimumSize().y;

		if (grid[a].getRowSpan() <= 1)
		{
			if (pref_height > rows[grid[a].getRow()].size)
				rows[grid[a].getRow()].size = pref_height;
			if (min_height > rows[grid[a].getRow()].min_size)
				rows[grid[a].getRow()].min_size = min_height;
		}
	}

	// Now process multi-row-spanned widgets
	for (unsigned a = 0; a < grid.size(); a++)
	{
		Widget* widget = grid[a].getWidget();

		if (grid[a].getRowSpan() <= 1 || !widget->isVisible())
			continue;

		int pref_height = widget->getPreferredSize().y;
		int min_height = widget->getMinimumSize().y;

		// Get total heights (pref/min) of spanned rows
		int total_size = 0;
		int total_min_size = 0;
		for (unsigned c = grid[a].getRow(); c < grid[a].getRow() + grid[a].getRowSpan(); c++)
		{
			total_size += rows[c].size;
			total_min_size += rows[c].min_size;
		}

		// Increase size of last spanned row if the total preferred size is too small
		if (total_size < pref_height)
			rows[grid[a].getRow() + grid[a].getRowSpan() - 1].size += (pref_height - total_size);

		// Increase minimum size of last spanned row if the total min size is too small
		if (total_min_size < min_height)
			rows[grid[a].getRow() + grid[a].getRowSpan() - 1].min_size += (min_height - total_min_size);
	}

	// Get the total height
	int total_height = 0;
	for (unsigned a = 0; a < rows.size(); a++)
		total_height += rows[a].size;
	total_height += (rows.size() - 1) * spacing.y;

	// Check if we need to squish rows
	if (max_height >= 0 && total_height > max_height)
	{
		// Squish rows to min size starting from the bottom,
		// until we get below the max height (or not, in which
		// case it'll just have to be cut off)
		for (int a = rows.size() - 1; a >= 0; a--)
		{
			int diff = rows[a].size - rows[a].min_size;
			rows[a].size = rows[a].min_size;
			total_height -= diff;

			if (total_height < max_height)
			{
				// Fit to max width
				rows[a].size += (max_height - total_height);
				break;
			}
		}
	}
	else if (max_height >= 0)
	{
		// We have width to spare, so evenly distribute it between stretchable rows
		int extra = max_height - total_height;

		if (!stretch_rows.empty())
		{
			unsigned adjust = extra / stretch_rows.size();
			for (unsigned a = 0; a < stretch_rows.size(); a++)
				rows[stretch_rows[a]].size += adjust;

			// Add rounding remainder to last stretchable row
			if (adjust * stretch_rows.size() < extra)
				rows[stretch_rows.back()].size += (extra - (adjust * stretch_rows.size()));
		}
	}

	// Finally, calculate column positions
	unsigned pos = padding.top;
	for (unsigned a = 0; a < rows.size(); a++)
	{
		rows[a].position = pos;
		pos += rows[a].size + spacing.y;
	}
	*/
}

void GridPanel::updateLayout()
{
	//calculateColumns(fit.x);
	//calculateRows(fit.y);

	// (Re)build column/row lists to fit all widgets
	unsigned ncols = 0;
	unsigned nrows = 0;
	for (unsigned a = 0; a < grid.size(); a++)
	{
		int cmax = grid[a].getColumn() + grid[a].getColumnSpan();
		if (cmax > ncols)
			ncols = cmax;

		int rmax = grid[a].getRow() + grid[a].getRowSpan();
		if (rmax > nrows)
			nrows = rmax;
	}
	columns.resize(ncols);
	rows.resize(nrows);

	// First up, determine minimum sizes for all columns/rows
	for (unsigned a = 0; a < grid.size(); a++)
	{
		Widget* widget = grid[a].getWidget();

		if (!widget->isVisible())
			continue;

		dim2_t min = dim2_t(0, 0);// widget->getMinimumSize();

		if (grid[a].getColumnSpan() <= 1 && min.x > columns[grid[a].getColumn()].min_size)
			columns[grid[a].getColumn()].min_size = min.x;
		if (grid[a].getRowSpan() <= 1 && min.y > rows[grid[a].getRow()].min_size)
			rows[grid[a].getRow()].min_size = min.y;
	}

	// Process multi-column-spanned widgets
	for (unsigned a = 0; a < grid.size(); a++)
	{
		Widget* widget = grid[a].getWidget();

		if (grid[a].getColumnSpan() <= 1 || !widget->isVisible())
			continue;

		int min_width = 0;// widget->getMinimumSize().x;

		// Get total widths (pref/min) of spanned columns
		int total_min_size = 0;
		for (unsigned c = grid[a].getColumn(); c < grid[a].getColumn() + grid[a].getColumnSpan(); c++)
			total_min_size += columns[c].min_size;

		// Increase minimum size of last spanned column if the total min size is too small
		if (total_min_size < min_width)
			columns[grid[a].getColumn() + grid[a].getColumnSpan() - 1].min_size += (min_width - total_min_size);
	}

	// Process multi-row-spanned widgets
	for (unsigned a = 0; a < grid.size(); a++)
	{
		Widget* widget = grid[a].getWidget();

		if (grid[a].getRowSpan() <= 1 || !widget->isVisible())
			continue;

		int min_height = 0;// widget->getMinimumSize().y;

		// Get total heights (pref/min) of spanned rows
		int total_min_size = 0;
		for (unsigned c = grid[a].getRow(); c < grid[a].getRow() + grid[a].getRowSpan(); c++)
			total_min_size += rows[c].min_size;

		// Increase minimum size of last spanned row if the total min size is too small
		if (total_min_size < min_height)
			rows[grid[a].getRow() + grid[a].getRowSpan() - 1].min_size += (min_height - total_min_size);
	}
}

void GridPanel::applyLayout(dim2_t fit)
{
	calculateColumns(fit.x);
	calculateRows(fit.y);

	for (unsigned a = 0; a < grid.size(); a++)
	{
		if (!grid[a].getWidget()->isVisible())
			continue;

		column_t& col = columns[grid[a].getColumn()];
		row_t& row = rows[grid[a].getRow()];

		int col_width = col.size;
		int row_height = row.size;

		// Multi column
		if (grid[a].getColumnSpan() > 1)
		{
			col_width = 0;
			for (unsigned c = grid[a].getColumn(); c < grid[a].getColumn() + grid[a].getColumnSpan(); c++)
				col_width += columns[c].size;
			//col_width += spacing.x * (grid[a].getColumnSpan() - 1);
		}

		// Multi row
		if (grid[a].getRowSpan() > 1)
		{
			row_height = 0;
			for (unsigned c = grid[a].getRow(); c < grid[a].getRow() + grid[a].getRowSpan(); c++)
				row_height += rows[c].size;
			//height += spacing.y * (grid[a].getRowSpan() - 1);
		}

		Widget* widget = grid[a].getWidget();
		int x = 0;
		int y = 0;

		// Align horizontally
		switch (grid[a].getHorizontalAnchor())
		{
		case GridPos::ANCHOR_RIGHT: x = col.position + col_width - widget->getWidth(); break;
		case GridPos::ANCHOR_CENTER: x = col.position + (col_width / 2) - (widget->getWidth() / 2); break;
		default: x = col.position;
		}

		// Align vertically
		switch (grid[a].getVerticalAnchor())
		{
		case GridPos::ANCHOR_BOTTOM: y = row.position + row_height - widget->getHeight(); break;
		case GridPos::ANCHOR_CENTER: y = row.position + (row_height / 2) - (widget->getHeight() / 2); break;
		default: y = row.position;
		}

		widget->setPosition(point2_t(x, y));
		dim2_t pref_size = dim2_t(0, 0);// widget->getPreferredSize(dim2_t(col_width, row_height));
		widget->setSize(
			dim2_t(
			min(col_width, pref_size.x),
			min(row_height, pref_size.y)
			));

		// Fill
		if (grid[a].getHorizontalAnchor() == GridPos::ANCHOR_FILL)
			widget->setSize(dim2_t(col_width, widget->getHeight()));
		if (grid[a].getVerticalAnchor() == GridPos::ANCHOR_FILL)
			widget->setSize(dim2_t(widget->getWidth(), row_height));
	}
}

void GridPanel::drawGrid(point2_t pos)
{
	int count = 0;
	for (unsigned c = 0; c < columns.size(); c++)
	{
		column_t& col = columns[c];
		for (unsigned r = 0; r < rows.size(); r++)
		{
			row_t& row = rows[r];

			if (count % 2 == 0)
				OpenGL::setColour(150, 150, 150, 150, 0);
			else
				OpenGL::setColour(80, 80, 80, 150, 0);

			Drawing::drawFilledRect(pos.x + col.position, pos.y + row.position,
				pos.x + col.position + col.size, pos.y + row.position + row.size);

			if (spacing.x == 0 && spacing.y == 0)
				count++;
		}
	}
}
