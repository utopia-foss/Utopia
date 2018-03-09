#ifndef HDFFILE_HH
#define HDFFILE_HH
#include "hdfdataset.hh"
#include "hdfgroup.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <sstream>
#include <stdexcept>
#include <string>

namespace Utopia {
namespace DataIO {
// forward declaration of HDFGroup -> exchange later for the real thing.

class HDFFile {
protected:
    hid_t _file;
    std::string _path;

    std::shared_ptr<HDFGroup> _base_group;

public:
    /**
     * @brief Function for exchanging states
     *
     * @param other
     */
    void swap(HDFFile &other) {
        using Utopia::DataIO::swap;
        using std::swap;
        swap(_file, other._file);
        swap(_path, other._path);
        swap(_base_group, other._base_group);
    }

    /**
     * @brief closes the hdffile
     *
     */
    void close() {
        if (H5Iis_valid(_file) == true) {
            H5Fflush(_file, H5F_SCOPE_GLOBAL);

            H5Fclose(_file);
        }
    }

    /**
     * @brief Get the id object
     *
     * @return hid_t
     */
    hid_t get_id() { return _file; }

    /**
     * @brief Get the path object
     *
     * @return std::string
     */
    std::string get_path() { return _path; }

    /**
     * @brief Get the basegroup object via shared ptr
     *
     * @return std::shared_ptr<HDFGroup>
     */
    std::shared_ptr<HDFGroup> get_basegroup() { return _base_group; }

    /**
     * @brief Open group at path 'path', creating all intermediate objects in
     * the path
     *
     * @param path
     * @return std::shared_ptr<HDFGroup>
     */
    std::shared_ptr<HDFGroup> open_group(std::string &&path) {
        std::stringstream stream(path);
        std::string part;
        auto current = std::make_shared<HDFGroup>();
        auto parent = _base_group;
        while (std::getline(stream, part, '/')) {
            if (part == "") {
                continue;
            }
            current = std::make_shared<HDFGroup>(*parent, *this, part);
            parent = current;
        }
        return current;
    }

    /**
     * @brief deletes the group pointed to by absolute path 'path'
     *
     * @param path absolute path to group to delete
     */

    void delete_group(std::string &&path) {
        // check if group exists in file, if does delete link
        herr_t status = 1;
        if (H5Lexists(_file, path.c_str(), H5P_DEFAULT) == true) {
            // group exists, can be deleted

            hid_t group_to_delete = H5Gopen(_file, path.c_str(), H5P_DEFAULT);
            if (H5Iis_valid(group_to_delete)) {
                status = H5Ldelete(group_to_delete, path.c_str(), H5P_DEFAULT);
                H5Gclose(group_to_delete);
            }
            if (status < 0) {
                close();
                throw std::runtime_error(
                    "deletion of group failed, wrong path?");
            }
        }
        // group does not exist, do nothing
    }

    /**
     * @brief Initiates an immediate write to disk of the data of the file
     *
     */
    void flush() {
        if (H5Iis_valid(_file) == true) {
            H5Fflush(_file, H5F_SCOPE_GLOBAL);
        }
    }

    /**
     * @brief Construct a new default HDFFile object
     *
     */
    HDFFile() = default;
    /**
     * @brief Move constructor Construct a new HDFFile object via move semantics
     *
     * @param other rvalue reference to HDFFile object
     */
    HDFFile(HDFFile &&other) : HDFFile() { this->swap(other); }

    /**
     * @brief Move assigment operator
     *
     * @param rvalue reference to HDFFile object
     * @return HDFFile&
     */
    HDFFile &operator=(HDFFile other) {
        this->swap(other);
        return *this;
    }

    HDFFile(const HDFFile &other)
        : _file(other._file), _path(other._path),
          _base_group(other._base_group) {}

    /**
     * @brief Construct a new HDFFile object
     *
     * @param path Path to the new file
     * @param access Access specifier for the new file, can be:
     *   r 	    Readonly, file must exist
     *   r+ 	Read/write, file must exist
     *   w 	    Create file, truncate if exists
     *   x 	    Create file, fail if exists
     *   a 	   Read/write if exists, create otherwise (default)
     */
    HDFFile(std::string path, std::string access)
        : _file([&]() {
              if (access == "w") {
                  return H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT,
                                   H5P_DEFAULT);
              } else if (access == "r") {
                  return H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
              } else if (access == "r+") {
                  return H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);

              } else if (access == "x") {
                  hid_t file = H5Fcreate(path.c_str(), H5F_ACC_EXCL,
                                         H5P_DEFAULT, H5P_DEFAULT);
                  if (file < 0) {
                      throw std::runtime_error("tried to create an existing "
                                               "file in non-truncate mode");

                  } else {
                      return file;
                  }
              } else if (access == "a") {
                  hid_t file_test =
                      H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
                  if (file_test < 0) {
                      H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT,
                                H5P_DEFAULT);
                  }
                  return file_test;

              } else {
                  throw std::invalid_argument("wrong type of access specifier, "
                                              "see documentation for allowed "
                                              "values");
              }
          }()),
          _path(path),
          _base_group(std::make_shared<HDFGroup>(*this, *this, "/")) {}

    /**
     * @brief Destroy the HDFFile object
     *
     */
    virtual ~HDFFile() {
        if (H5Iis_valid(_file) == true) {
            H5Fflush(_file, H5F_SCOPE_GLOBAL);
            H5Fclose(_file);
        }
    }
};
} // namespace DataIO
} // namespace Utopia
#endif