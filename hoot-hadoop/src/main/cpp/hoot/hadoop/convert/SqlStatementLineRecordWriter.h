/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef SQLSTATEMENTLINERECORDWRITER_H
#define SQLSTATEMENTLINERECORDWRITER_H

// Pretty Pipes
#include <pp/io/LineRecordWriter.h>
using namespace pp;

// std
#include <string>

namespace hoot
{

using namespace std;

/**
 * Identical to LineRecordWriter except for not outputting a tab between the key and value pair
 */
class SqlStatementLineRecordWriter : public LineRecordWriter
{

public:

  static string className() { return "hoot::SqlStatementLineRecordWriter"; }

  SqlStatementLineRecordWriter();

  virtual ~SqlStatementLineRecordWriter() {};

  virtual void emitRecord(const char* keyData, size_t keySize, const char* valueData,
                          size_t valueSize);

};

}

#endif // SQLSTATEMENTLINERECORDWRITER_H