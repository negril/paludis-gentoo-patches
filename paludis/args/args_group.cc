/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "args.hh"
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <list>

/** \file
 * Implementation for ArgsGroup.
 *
 * \ingroup grplibpaludisargs
 */

using namespace paludis::args;

namespace paludis
{
    /**
     * Implementation data for ArgsGroup.
     *
     * \ingroup grplibpaludisargs
     */
    template<>
    struct Implementation<ArgsGroup>
    {
        std::list<ArgsOption *> args_options;
    };
}

ArgsGroup::ArgsGroup(ArgsHandler * h, const std::string & our_name,
        const std::string & our_description) :
    PrivateImplementationPattern<ArgsGroup>(new Implementation<ArgsGroup>),
    _name(our_name),
    _description(our_description),
    _handler(h)
{
    h->add(this);
}

void
ArgsGroup::add(ArgsOption * const value)
{
    /// \bug Should check for uniqueness of short and long names.
    _imp->args_options.push_back(value);
}

ArgsGroup::~ArgsGroup()
{
}

ArgsGroup::ConstIterator
ArgsGroup::begin() const
{
    return ConstIterator(_imp->args_options.begin());
}

ArgsGroup::ConstIterator
ArgsGroup::end() const
{
    return ConstIterator(_imp->args_options.end());
}

