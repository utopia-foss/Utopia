#ifndef HDFGROUP_HH
#define HDFGROUP_HH

#include <string>
#include <sstream>
#include <unordered_map>

#include <hdf5.h>
#include <hdf5_hl.h>
#include "hdfmockclasses.hh"
#include "hdfdataset.hh"

namespace Utopia
{
    
namespace DataIO
{

class HDFFile;

/// An object representing a HDFGroup 
/** 
 * 
*/
class HDFGroup
{
protected:
    hid_t _group; /// Storage of group id
    std::string _path; /// Storage of the path

public:
    void info()
    {
        // TODO
        H5G_info_t* info;
        H5Gget_info(_group, info);


        std::cout << "Printing information of the group:" << std::endl
            << "- Group id: " << _group
            << "- Group path: " << _path
            << "- Number of links in group: " << info->nlinks 
            << "- Current maximum creation order value for group: " << info->max_corder
            << "- There are mounted files on the group: " << info->mounted
            << std::endl; 
    }

    /// Get the id
    /**
    *  \return The hdf5 id
    */
    hid_t get_id()
    {
        return _group;
    }

    /// Get the path
    /**
    *  \return The hdf5 path
    */
    std::string get_path()
    {
        return _path;
    }

    /// Add an attribute to the HDFGroup object
    /** Adds an attribute to a group
    *  \tparam Attrdata Attribute type 
    *  \param name Attribute name
    *  \param attribute_data Attribute data
    */
    template <typename Attrdata>
    void add_attribute(std::string name, Attrdata attribute_data) {

        HDFAttribute<HDFGroup, Attrdata> attribute(
            *this, std::forward<std::string &&>(name));

        attribute.write(attribute_data);
    }

    /// Open a new HDFGroup
    /**
    * \param path The path
    * \return A pointer to the newly created HDFGroup
    */
    std::shared_ptr<HDFGroup> open_group(std::string path)
    {
        return std::make_shared<HDFGroup>(HDFGroup(*this, path));
    }

    /// Close the group if there are no more child groups or datasets
    void close() 
    {
        if (H5Iis_valid(_group) == 0)
        {
            H5Gclose(_group); 
        } 
    }

    /// Open a HDFDataset
    /** 
    *  
    */
    std::shared_ptr<HDFDataset<HDFGroup>>  open_dataset(std::string path)
    {
        auto data = HDFDataset<HDFGroup>(*this, path);
        auto data_ptr = std::make_shared<HDFDataset<HDFGroup>>(data);
        return std::make_shared<HDFDataset<HDFGroup>>(data);
    }

    /// Swap two HDFGroup objects
    /**
    *  \param group1 First HDFGroup object
    *  \param group2 Second HDFGroup object
    */
    friend void swap(HDFGroup& group1, HDFGroup& group2)
    {
        using std::swap;

        swap(group1._group, group2._group);
        swap(group1._path, group2._path);
    }

    /// Default constructor
    HDFGroup() = default;

    /// Copy constructor
    /** Constructs an HDFGroup object by copying another HDFGroup object
    *  \param group The HDFGroup object that is copied
    */
    HDFGroup(const HDFGroup& group) :   _group(group._group),
                                        _path(group._path)
    {};

    /// Move constructor
    /** 
    * \param group The rvalue reference
    */
    HDFGroup(HDFGroup&& group):     _group(std::move(group._group)),
                                    _path(std::move(group._path))
    {};

    /// Assign a HDFGroup object using an assignment operator
    /** Implement assignment operation using the copy-and-swap idiom
    *
    *  \return the copied HDFGroup object
    */ 
    HDFGroup& operator=(HDFGroup group)
    {
        swap(group, *this);
        return *this;
    }
    
    /// Constructor
    template <typename Object>
    HDFGroup(Object &object, std::string name) : _path(name) 
    {
        if (std::is_same<Object, HDFFile>::value) 
        {
            if (_path == "/") 
            {
                _group = H5Gopen(object.get_id(), "/", H5P_DEFAULT);
            } else 
            {
                if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) == 1) 
                {
                    _group =
                        H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
                } else 
                {
                    _group = H5Gcreate(object.get_id(), _path.c_str(),
                                       H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                }
            }
        } else if (std::is_same<Object, HDFGroup>::value) 
        {
            if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) == 1) 
            {
                _group = H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
            } else 
            {
                _group = H5Gcreate(object.get_id(), _path.c_str(), H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT);
            }
        }
    }

    /// Destructor
    virtual ~HDFGroup()
    {
        if (H5Iis_valid(_group) == 0) { 
            H5Gclose(_group);
        }
    }

}; // class HDFGroup

} // namespace DataIO 

} // namespace Utopia

#endif // HDFGROUP_HH
