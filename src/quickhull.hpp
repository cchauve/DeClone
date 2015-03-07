/*   Quickhull algorithm implementation
 **
 ** Copyright (c) 2014-2015, Anatoliy V. Tomilov
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following condition is met:
 ** Redistributions of source code must retain the above copyright notice, this condition and the following disclaimer.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 ** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 ** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 ** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 ** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 ** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 ** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 ** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 ** POSSIBILITY OF SUCH DAMAGE.
 **/
#pragma once

#include <valarray>
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <utility>
#include <numeric>

#include <cmath>
#include <cassert>

template< class point_iterator >
struct quick_hull
{

    using size_type = std::size_t;

    //static_assert(std::is_base_of< std::random_access_iterator_tag, typename std::iterator_traits< point_iterator >::iterator_category >::value, "Error Msg");

    using point = typename std::iterator_traits< point_iterator >::value_type;
    using value_type = typename point::value_type;

    size_type const dimension_;
    value_type const eps;

    quick_hull(size_type const _dimension,
               value_type _eps = std::numeric_limits< value_type >::epsilon())
        : dimension_(_dimension)
        , eps(std::move(_eps))
        , matrix_(_dimension)
        , shadow_matrix_(_dimension)
        , minor_()
        , origin_(zero, _dimension)
    {
        assert(1 < dimension_);
        assert(!(eps < zero));
        size_type const minor_size = dimension_ - 1;
        minor_.resize(minor_size);
        for (size_type r = 0; r < minor_size; ++r) {
            matrix_[r].resize(dimension_);
            shadow_matrix_[r].resize(dimension_);
            minor_[r].resize(minor_size);
        }
        matrix_.back().resize(dimension_);
        shadow_matrix_.back().resize(dimension_);
    }

    using point_array = std::vector< point_iterator >;
    using point_list = std::list< point_iterator >;
    using facet_array = std::vector< size_type >;

    struct facet // (d - 1)-dimensional face
    {

        using normal = std::valarray< value_type >;

        point_array vertices_; // d points (oriented)
        point_list outside_; // if empty, then is convex hull's facet, else the first point (i.e. outside_.front()) is the furthest point from this facet
        facet_array neighbours_; // neighbouring facets

        // hyperplane equation
        normal normal_; // components of normalized normal vector
        value_type D; // distance from the origin to the hyperplane

        void
        init(point_array && _vertices,
             size_type const _neighbour)
        {
            vertices_ = std::move(_vertices);
            size_type const dimension_ = vertices_.size();
            neighbours_.reserve(dimension_);
            neighbours_.push_back(_neighbour);
            normal_.resize(dimension_);
        }

        facet(point_array && _vertices,
              size_type const _neighbour)
        {
            init(std::move(_vertices), _neighbour);
        }

        facet(typename point_list::const_iterator _first,
              typename point_list::const_iterator _middle,
              typename point_list::const_iterator _last)
            : vertices_(_first, std::prev(_middle))
        {
            vertices_.insert(std::end(vertices_), _middle, _last);
            size_type const dimension_ = vertices_.size();
            neighbours_.reserve(dimension_);
            normal_.resize(dimension_);
        }

        value_type
        distance(point const & _point) const
        {
            return std::inner_product(std::begin(normal_), std::end(normal_), std::begin(_point), D);
        }

    };

    using facets_storage = std::deque< facet >;

    facets_storage facets_;

    value_type
    cos_of_dihedral_angle(facet const & _this, facet const & _other) const
    {
        return std::inner_product(std::begin(_this.normal_), std::end(_this.normal_), std::begin(_other.normal_), zero);
    }

private :

    // math (simple functions, matrices, etc):

    value_type const zero = value_type(0);
    value_type const one = value_type(1);

    using row = std::valarray< value_type >;
    using matrix = std::vector< row >;

    matrix matrix_;
    matrix shadow_matrix_;
    matrix minor_;
    row origin_;

    void
    transpose()
    { // transpose to cheaper filling columns with ones
        for (size_type r = 0; r < dimension_; ++r) {
            row & row_ = shadow_matrix_[r];
            for (size_type c = 1 + r; c < dimension_; ++c) {
                using std::swap;
                swap(shadow_matrix_[c][r], row_[c]);
            }
        }
    }

    void
    restore_matrix()
    {
        matrix_ = shadow_matrix_;
    }

    void
    restore_matrix(size_type const _identity) // load matrix from storage and replace _identity column with ones
    {
        for (size_type c = 0; c < dimension_; ++c) {
            row & col_ = matrix_[c];
            if (c == _identity) {
                col_ = one;
            } else {
                col_ = shadow_matrix_[c];
            }
        }
    }

    void
    square_matrix(size_type const _size) // matrix_ = shadow_matrix_ * transposed shadow_matrix_
    {
        assert(_size < dimension_);
        for (size_type r = 0; r < _size; ++r) {
            row & lhs_ = shadow_matrix_[r];
            row const & row_ = matrix_[r];
            auto const rbeg = std::begin(row_);
            auto const rend = std::end(row_);
            for (size_type c = 0; c < _size; ++c) {
                lhs_[c] = std::inner_product(rbeg, rend, std::begin(matrix_[c]), zero);
            }
        }
    }

    value_type
    det(matrix & _matrix, size_type const _size) // based on LUP decomposition (complexity is 2 * n^3 / 3 + O(n^2) vs 4 * n^3 / 3 + O(n^2) for QR decomposition via Householder reflections)
    {
        value_type det_ = one;
        for (size_type i = 0; i < _size; ++i) {
            size_type p = i;
            using std::abs;
            value_type max_ = abs(_matrix[p][i]);
            size_type pivot = p;
            while (++p < _size) {
                value_type y_ = abs(_matrix[p][i]);
                if (max_ < y_) {
                    max_ = std::move(y_);
                    pivot = p;
                }
            }
            if (!(eps < max_)) { // regular?
                return zero; // singular
            }
            row & ri_ = _matrix[i];
            if (pivot != i) {
                det_ = -det_; // each permutation flips sign of det
                ri_.swap(_matrix[pivot]);
            }
            value_type & dia_ = ri_[i];
            value_type const inv_ = one / dia_;
            det_ *= std::move(dia_); // det is multiple of diagonal elements
            for (size_type j = 1 + i; j < _size; ++j) {
                _matrix[j][i] *= inv_;
            }
            for (size_type a = 1 + i; a < _size; ++a) {
                row & a_ = minor_[a - 1];
                value_type const & ai_ = _matrix[a][i];
                for (size_type b = 1 + i; b < _size; ++ b) {
                    a_[b - 1] = ai_ * ri_[b];
                }
            }
            for (size_type a = 1 + i; a < _size; ++a) {
                row const & a_ = minor_[a - 1];
                row & ra_ = _matrix[a];
                for (size_type b = 1 + i; b < _size; ++ b) {
                    ra_[b] -= a_[b - 1];
                }
            }
        }
        return det_;
    }

    value_type
    det()
    {
        return det(matrix_, dimension_);
    }

    // geometry and basic operations on geometric primitives:

    struct ridgelike
    {

        point_array const & ordered_;
        size_type const facet_;
        size_type const excluded_vertex_;

        bool
        operator < (ridgelike const & _other_ridge) const
        {
            size_type const dimension_ = ordered_.size();
            size_type i = 0;
            size_type j = 0;
            for (;;) {
                if (i == excluded_vertex_) {
                    ++i;
                }
                if (j == _other_ridge.excluded_vertex_) {
                    ++j;
                }
                if (i == dimension_) {
                    assert(j == dimension_);
                    break;
                }
                if (j == dimension_) {
                    assert(i == dimension_);
                    break;
                }
                if (ordered_[i] < _other_ridge.ordered_[j]) {
                    return true;
                } else if (_other_ridge.ordered_[j] < ordered_[i]) {
                    return false;
                } else {
                    ++i;
                    ++j;
                }
            }
            return false;
        }

    };

    std::deque< point_array > ordered_; // ordered, but not oriented vertices of facets
    std::set< ridgelike > unique_ridges_;

    void
    find_adjacent_facets(size_type const _facet, point_iterator const _apex)
    {
        for (size_type i = 0; i < dimension_; ++i) {
            point_array const & ridgelike_ = ordered_[_facet];
            if (ridgelike_[i] != _apex) {
                auto position = unique_ridges_.insert({ridgelike_, _facet, i});
                if (!position.second) {
                    size_type const neighbour = position.first->facet_;
                    facets_[neighbour].neighbours_.push_back(_facet);
                    facets_[_facet].neighbours_.push_back(neighbour);
                    unique_ridges_.erase(position.first);
                }
            }
        }
    }

    void
    set_hyperplane_equation(facet & _facet)
    {
        for (size_type r = 0; r < dimension_; ++r) {
            std::copy_n(std::begin(*_facet.vertices_[r]), dimension_, std::begin(shadow_matrix_[r]));
        }
        transpose();
        value_type N = zero;
        for (size_type i = 0; i < dimension_; ++i) {
            restore_matrix(i);
            value_type n = det();
            N += n * n;
            _facet.normal_[i] = std::move(n);
        }
        using std::sqrt;
        N = sqrt(N);
        restore_matrix();
        _facet.normal_ /= N;
        _facet.D = -det() / std::move(N);
    }

    std::set< size_type, std::greater< size_type > > removed_facets_;

    size_type
    add_facet(point_array && _vertices, size_type const _neighbour)
    {
        assert(ordered_.size() == facets_.size());
        if (removed_facets_.empty()) {
            size_type const f = facets_.size();
            facets_.emplace_back(std::move(_vertices), _neighbour);
            facet & facet_ = facets_.back();
            set_hyperplane_equation(facet_);
            ordered_.emplace_back();
            point_array & ordered_vertices_ = ordered_.back();
            ordered_vertices_ = facet_.vertices_;
            std::sort(std::begin(ordered_vertices_), std::end(ordered_vertices_));
            return f;
        } else {
            auto const rend = std::prev(std::end(removed_facets_));
            size_type const f = *rend;
            removed_facets_.erase(rend);
            facet & facet_ = facets_[f];
            facet_.init(std::move(_vertices), _neighbour);
            set_hyperplane_equation(facet_);
            point_array & ordered_vertices_ = ordered_[f];
            ordered_vertices_ = facet_.vertices_;
            std::sort(std::begin(ordered_vertices_), std::end(ordered_vertices_));
            return f;
        }
    }

    // http://math.stackexchange.com/questions/822741/
    value_type
    hypervolume(point_list const & _vertices)
    { // volume of conv(_vertices)
        assert(!_vertices.empty());
        size_type const rank_ = _vertices.size() - 1;
        assert(!(dimension_ < rank_));
        assert(!_vertices.empty());
        std::copy_n(std::begin(*_vertices.back()), dimension_, std::begin(origin_));
        auto vertex = std::begin(_vertices);
        for (size_type r = 0; r < rank_; ++r) { // affine space -> vector space
            row & row_ = matrix_[r];
            std::copy_n(std::begin(**vertex), dimension_, std::begin(row_));
            row_ -= origin_;
            ++vertex;
        }
        if (rank_ == dimension_) { // oriented hypervolume
            return det();
        } else { // non-oriented _rank-dimensional measure
            square_matrix(rank_);
            using std::sqrt;
            return sqrt(det(shadow_matrix_, rank_));
        }
    }

    void
    orthogonalize(point_list const & _affine_space, size_type const _rank)
    {
        assert(!(dimension_ < _rank));
        auto vertex = std::begin(_affine_space);
        for (size_type r = 0; r < _rank; ++r) { // affine space -> vector space
            row & row_ = shadow_matrix_[r];
            std::copy_n(std::begin(**vertex), dimension_, std::begin(row_));
            row_ -= origin_;
            ++vertex;
        }
        for (size_type i = 0; i < _rank; ++i) { // Householder transformation
            value_type norm_ = zero;
            row & qri_ = shadow_matrix_[i]; // shadow_matrix_ is packed QR after
            for (size_type j = i; j < dimension_; ++j) {
                value_type const & qrij_ = qri_[j];
                norm_ += qrij_ * qrij_;
            }
            using std::sqrt;
            norm_ = sqrt(norm_);
            assert(eps < norm_);
            value_type & qrii_ = qri_[i];
            bool const sign_ = (zero < qrii_);
            value_type factor_ = norm_ * (norm_ + (sign_ ? qrii_ : -qrii_));
            assert(eps < factor_);
            factor_ = one / sqrt(std::move(factor_));
            if (sign_) {
                qrii_ += norm_;
            } else {
                qrii_ -= norm_;
            }
            for (size_type k = i; k < dimension_; ++k) {
                qri_[k] *= factor_;
            }
            for (size_type j = i + 1; j < _rank; ++j) {
                row & qrj_ = shadow_matrix_[j];
                value_type s_ = zero;
                for (size_type k = i; k < dimension_; ++k) {
                    s_ += qri_[k] * qrj_[k];
                }
                for (size_type k = i; k < dimension_; ++k) {
                    qrj_[k] -= qri_[k] * s_;
                }
            }
        }
        for (size_type i = 0; i < _rank; ++i) { // calculation of Q
            row & qi_ = matrix_[i]; // matrix_ is Q after
            qi_ = zero;
            qi_[i] = one;
            size_type j = _rank;
            while (0 < j) {
                --j;
                row & qrj_ = shadow_matrix_[j];
                value_type s_ = zero;
                for (size_type k = j; k < dimension_; ++k) {
                    s_ += qrj_[k] * qi_[k];
                }
                for (size_type k = j; k < dimension_; ++k) {
                    qi_[k] -= qrj_[k] * s_;
                }
            }
        }
    }

    bool
    steal_best(point_list & _from, point_list & _to)
    {
        assert(!_to.empty());
        std::copy_n(std::begin(*_to.back()), dimension_, std::begin(origin_));
        size_type const rank_ = _to.size() - 1;
        orthogonalize(_to, rank_);
        row & projection_ = shadow_matrix_.back(); // projection to orghogonal subspace
        row & apex_ = shadow_matrix_.front();
        auto const abeg = std::begin(apex_);
        auto const aend = std::end(apex_);
        value_type distance_ = zero;
        auto const end = std::end(_from);
        auto furthest = end;
        for (auto it = std::begin(_from); it != end; ++it) {
            std::copy_n(std::begin(**it), dimension_, abeg);
            apex_ -= origin_; // turn translated space into vector space
            projection_ = apex_;
            for (size_type i = 0; i < rank_; ++i) {
                row const & qi_ = matrix_[i];
                projection_ -= std::inner_product(abeg, aend, std::begin(qi_), zero) * qi_;
            }
            projection_ *= projection_;
            using std::sqrt;
            value_type d_ = sqrt(projection_.sum()); // distance to subspace
            if (distance_ < d_) {
                distance_ = std::move(d_);
                furthest = it;
            }
        }
        if (furthest == end) {
            return false;
        }
        _to.splice(std::end(_to), _from, furthest);
        return true;
    }

    using ranking = std::multimap< value_type, size_type >;
    using ranking_meta = std::unordered_map< size_type, typename ranking::iterator >;

    ranking ranking_;
    ranking_meta ranking_meta_;

    void
    rank(value_type && _orientation, size_type const _facet)
    {
        if (eps < _orientation) {
            ranking_meta_.emplace(_facet, ranking_.emplace(std::move(_orientation), _facet));
        }
    }

    void
    unrank_and_remove(size_type const _facet)
    {
        auto const r = ranking_meta_.find(_facet);
        if (r != std::end(ranking_meta_)) {
            ranking_.erase(r->second);
            ranking_meta_.erase(r);
        }
        removed_facets_.insert(_facet);
    }

    size_type
    get_best_facet() const
    {
        if (ranking_.empty()) {
            return facets_.size(); // special value
        } else {
            return std::prev(std::end(ranking_))->second;
        }
    }

    value_type
    partition(facet & _facet, point_list & _points)
    {
        auto it = std::begin(_points);
        auto const end = std::end(_points);
        value_type distance_ = eps;
        while (it != end) {
            auto const next = std::next(it);
            value_type d_ = _facet.distance(**it);
            if (distance_ < d_) {
                distance_ = std::move(d_);
                _facet.outside_.splice(std::begin(_facet.outside_), _points, it);
            } else if (eps < d_) {
                _facet.outside_.splice(std::end(_facet.outside_), _points, it);
            }
            it = next;
        }
        return distance_;
    }

    using facet_unordered_set = std::unordered_set< size_type >;

    facet_unordered_set visited_;
    facet_unordered_set bth_facets_;
    facet_unordered_set not_bth_facets_;

    bool
    is_invisible(size_type const _facet) const
    {
        return (0 == not_bth_facets_.count(_facet)) && (0 == bth_facets_.count(_facet));
    }

    bool
    process_visibles(size_type const _facet, point const & _apex)
    {
        visited_.insert(_facet);
        facet const & facet_ = facets_[_facet];
        if (eps < facet_.distance(_apex)) {
            bool bth_ = false;
            for (size_type const neighbour : facet_.neighbours_) {
                if (visited_.count(neighbour) == 0) {
                    if (process_visibles(neighbour, _apex)) {
                        bth_ = true;
                    }
                } else if (!bth_) {
                    if (is_invisible(neighbour)){
                        bth_ = true;
                    }
                }
            }
            if (bth_) {
                bth_facets_.insert(_facet);
            } else {
                not_bth_facets_.insert(_facet);
            }
            return false;
        } else {
            return true;
        }
    }

    void
    clear_bth()
    {
        visited_.clear();
        bth_facets_.clear();
        not_bth_facets_.clear();
    }

    void
    replace_neighbour(size_type const _facet, size_type const _from, size_type const _to)
    {
        if (_from == _to) {
            return;
        }
        for (size_type & neighbour : facets_[_facet].neighbours_) {
            if (neighbour == _from) {
                neighbour = _to;
                return;
            }
        }
    }

public : // largest possible simplex heuristic, convex hull algorithm

    point_list
    create_simplex(point_iterator beg, point_iterator end)
    {
        // selection of (dimension_ + 1) affinely independent points
        point_list basis_;
        if (beg == end) {
            return basis_;
        }
        basis_.push_back(beg);
        point_list internal_set_;
        {
            auto it = beg;
            while (++it != end) {
                internal_set_.push_back(it);
            }
        }
        if (!steal_best(internal_set_, basis_)) {
            return basis_; // can't find affine independent second point
        }
        internal_set_.splice(std::end(internal_set_), basis_, std::begin(basis_)); // rejudge 0-indexed point
        for (size_type i = 0; i < dimension_; ++i) {
            if (!steal_best(internal_set_, basis_)) {
                return basis_; // can't find (i + 2) affine independent point
            }
        }
        assert(basis_.size() == dimension_ + 1); // simplex
        // simplex construction
        bool inward_ = (zero < hypervolume(basis_)); // is top oriented?
        auto const vbeg = std::begin(basis_);
        auto const vend = std::end(basis_);
        for (auto exclusive = vend; exclusive != vbeg; --exclusive) {
            size_type const newfacet = facets_.size();
            facets_.emplace_back(vbeg, exclusive, vend);
            facet & facet_ = facets_.back();
            inward_ = !inward_;
            if (inward_) {
                std::swap(facet_.vertices_.front(), // not works for dimension_ == 1
                          facet_.vertices_.back());
            }
            set_hyperplane_equation(facet_);
            ordered_.emplace_back();
            point_array & ordered_vertices_ = ordered_.back();
            ordered_vertices_ = facet_.vertices_;
            std::sort(std::begin(ordered_vertices_), std::end(ordered_vertices_));
            rank(partition(facet_, internal_set_), newfacet);
        }
        { // adjacency
            for (size_type i = 0; i <= dimension_; ++i) {
                facet_array & neighbours_ = facets_[i].neighbours_;
                for (size_type j = 0; j <= dimension_; ++j) {
                    if (i != j) {
                        neighbours_.push_back(j);
                    }
                }
            }
        }
        return basis_;
    }

    void
    create_convex_hull()
    {
        assert(facets_.size() == dimension_ + 1);
        assert(ordered_.size() == dimension_ + 1);
        assert(removed_facets_.empty());
        point_list outside_;
        point_array vertices_;
        facet_array neighbours_;
        point_array ridge_; // horizon ridge + furthest point = new facet
        facet_array newfacets_;
        for (size_type best_facet = get_best_facet(); best_facet != facets_.size(); best_facet = get_best_facet()) {
            point_list & best_facet_outsides_ = facets_[best_facet].outside_;
            assert(!best_facet_outsides_.empty());
            point_iterator const apex = best_facet_outsides_.front();
            best_facet_outsides_.pop_front();
            process_visibles(best_facet, *apex);
            assert(outside_.empty());
            for (size_type const not_bth_facet : not_bth_facets_) {
                facet & facet_ = facets_[not_bth_facet];
                outside_.splice(outside_.end(), std::move(facet_.outside_));
                facet_.vertices_.clear();
                facet_.neighbours_.clear();
                unrank_and_remove(not_bth_facet);
            }
            assert(newfacets_.empty());
            for (size_type const bth_facet : bth_facets_) {
                facet & facet_ = facets_[bth_facet];
                outside_.splice(outside_.end(), std::move(facet_.outside_));
                vertices_ = std::move(facet_.vertices_);
                neighbours_ = std::move(facet_.neighbours_);
                unrank_and_remove(bth_facet);
                for (size_type const neighbour : neighbours_) {
                    if (is_invisible(neighbour)) {
                        point_array const & horizon_ = ordered_[neighbour];
                        {
                            assert(ridge_.empty());
                            ridge_.reserve(dimension_);
                            for (point_iterator const vertex : vertices_) { // facets intersection with keeping of points order as it is in visible facet
                                if (std::binary_search(std::begin(horizon_), std::end(horizon_), vertex)) {
                                    ridge_.push_back(vertex);
                                } else {
                                    ridge_.push_back(apex);
                                }
                            }
                            assert(ridge_.size() == dimension_); // facet
                        }
                        size_type const newfacet = add_facet(std::move(ridge_), neighbour);
                        newfacets_.push_back(newfacet);
                        replace_neighbour(neighbour, bth_facet, newfacet);
                        find_adjacent_facets(newfacet, apex);
                    }
                }
            }
            assert(unique_ridges_.empty());
            for (size_type const newfacet : newfacets_) {
                rank(partition(facets_[newfacet], outside_), newfacet);
            }
            newfacets_.clear();
            outside_.clear();
            clear_bth();
        }
        assert(outside_.empty());
        assert(ranking_.empty());
        assert(ranking_meta_.empty());
        { // compactify
            size_type source = facets_.size();
            for (size_type const destination : removed_facets_) {
                if (destination != --source) {
                    facet & facet_ = facets_[destination];
                    facet_ = std::move(facets_.back());
                    for (size_type const neighbour : facet_.neighbours_) {
                        replace_neighbour(neighbour, source, destination);
                    }
                }
                facets_.pop_back();
            }
            facets_.shrink_to_fit();
            removed_facets_.clear();
        }
        ordered_.clear();
        ordered_.shrink_to_fit();
    }

};
