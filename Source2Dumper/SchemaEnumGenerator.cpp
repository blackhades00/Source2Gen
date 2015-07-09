#include <fstream>
#include <algorithm>

#include "SchemaEnumGenerator.hpp"

#include "SchemaUtil.hpp"

SchemaEnumGenerator::SchemaEnumGenerator(CSchemaSystemTypeScope* typeScope)
	: m_typeScope(typeScope),
	m_generatedHeader("")
{
	
}

std::string& SchemaEnumGenerator::generate()
{
	std::ofstream out(std::string(m_typeScope->GetScopeName()) + "_enums" + ".hpp", std::ofstream::out);

	if (!out.is_open())
		return m_generatedHeader;

	m_generatedHeader.clear();
	m_generatedHeader += "#pragma once\n";

	fillEnumInfoList(m_typeScope, m_enums);

	std::sort(m_enums.begin(), m_enums.end(),
		[](CSchemaEnumInfo* a, CSchemaEnumInfo* b)
	{
		return !strstr(a->m_Name.data, "::") && strstr(b->m_Name.data, "::");
	});

	std::sort(m_enums.begin(), m_enums.end(),
		[](CSchemaEnumInfo* a, CSchemaEnumInfo* b)
	{
		return !strstr(a->m_Name.data, "::") && !strstr(b->m_Name.data, "::")
			&& std::string(a->m_Name.data) < std::string(b->m_Name.data);
	});

	//m_generatedHeader += generateDeclarations();

	for (CSchemaEnumInfo* i : m_enums)
	{
		if (strstr(i->m_Name.data, "::"))
			continue;

		Single enumGen(i);
		m_generatedHeader += enumGen.generate();
	}

	out << m_generatedHeader;
	out.close();

	return m_generatedHeader;
}

std::string SchemaEnumGenerator::generateDeclarations()
{
	std::string declarations;


	for (CSchemaEnumInfo* i : m_enums)
	{
		declarations += std::string("enum ") + i->m_Name.data;
		declarations += ";\n";
	}

	return declarations;
}

std::string SchemaEnumGenerator::Single::generateBegin()
{
	std::string beginOfEnum;

	if (!m_enumInfo || !m_enumInfo->m_Name.data)
		return beginOfEnum;

	std::string baseName = m_enumInfo->m_Name.data;
	baseName = baseName.substr(baseName.find_last_of(":") + 1);

	// Generates a strongly typed enum.
	beginOfEnum += m_prefix + std::string("enum class ") + baseName;
	beginOfEnum += " : ";
	beginOfEnum += generateTypeStorage();
	beginOfEnum += "\n";

	beginOfEnum += m_prefix + "{\n";

	return beginOfEnum;
}

std::string SchemaEnumGenerator::Single::generateTypeStorage()
{
	std::string typeStorage = "";

	CSchemaType* enumType = m_enumInfo->getTypeScope()->FindSchemaTypeByName(m_enumInfo->m_Name.data);

	if (!enumType)
		return typeStorage;

	bool anyNegative = false;

	for (auto i = m_enumInfo->m_Enumerators.data; i != m_enumInfo->m_Enumerators.data + m_enumInfo->m_Enumerators.m_size; ++i)
	{
		if (i->m_nValue < 0)
		{
			anyNegative = true;
			break;
		}
	}

	std::string typePrefix = anyNegative ? "signed " : "unsigned ";

	switch (enumType->GetSize())
	{
	case 1:
		typeStorage = typePrefix + "char";
		break;
	case 2:
		typeStorage = typePrefix + "short";
		break;
	case 4:
		typeStorage = typePrefix + "long";
		break;
	case 8:
		typeStorage = typePrefix + "long long";
		break;
	default:
		typeStorage = typePrefix + "INVALID_TYPE";
	}

	return typeStorage;
}

std::string SchemaEnumGenerator::Single::generateFields()
{
	std::string fields;

	std::vector <SchemaEnumeratorInfoData_t*> enumFields;
	fillEnumFieldsList(m_enumInfo, enumFields);

	for (SchemaEnumeratorInfoData_t* i : enumFields)
	{
		if (i->m_Name.data)
		{
			//std::stringstream commentInfo;
			//commentInfo << std::hex << i->m_nSingleInheritanceOffset << " size " << std::dec << i->m_pType->getSize();

			fields += m_prefix;

			std::string baseName = i->m_Name.data;
			baseName = baseName.substr(baseName.find_last_of(":") + 1);

			fields += "\t";
			fields += baseName;
			fields += " = ";
			fields += std::to_string(i->m_nValue);
			fields += ",";
			//fields += "// " + commentInfo.str();
			fields += "\n";
		}
	}

	return fields;
}

std::string SchemaEnumGenerator::Single::generateEnd()
{
	return m_prefix + "};\n\n";
}

SchemaEnumGenerator::Single::Single(CSchemaEnumInfo* enumInfo, const std::string& prefix)
	: m_enumInfo(enumInfo),
	m_prefix(prefix)
{

}

std::string& SchemaEnumGenerator::Single::generate()
{
	m_generatedEnum = generateBegin() + generateFields() + generateEnd();
	return m_generatedEnum;
}