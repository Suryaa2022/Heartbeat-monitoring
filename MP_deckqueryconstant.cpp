#include "MP_DeckQueryConstant.h"

namespace lge {
namespace mm {

const std::string SubQuery_CDDASong =
    "SELECT 'CDDASong' as ObjectType "
        " , track_num as TrackNo "
        " , time_min  as TimeMin "
        " , time_sec  as TimeSec "
        " , frame     as Frame "
        " , title     as Title "
        " , player    as Player "
        " , lyricist  as Lyricist "
        " , composer  as Composer "
        " , arranger  as Arranger "
        " , message   as Message "
        " FROM CDDA ";

 const std::string Query_CDDASong =
    "SELECT CDDASong.ObjectType as ObjectType "
        " , CDDASong.TrackNo   as TrackNo "
        " , CDDASong.TimeMin   as TimeMin "
        " , CDDASong.TimeSec   as TimeSec "
        " , CDDASong.Frame     as Frame "
        " , CDDASong.Title     as Title "
        " , CDDASong.Player    as Player "
        " , CDDASong.Lyricist  as Lyricist "
        " , CDDASong.Composer  as Composer "
        " , CDDASong.Arranger  as Arranger "
        " , CDDASong.Message   as Message "
        " FROM "
            " ("
                + SubQuery_CDDASong +
            ") CDDASong";

 const std::string Query_CDDASongCount =
    "SELECT count(*) as TotalCount "
        " FROM "
            " ("
                + SubQuery_CDDASong +
            " ) CDDASong";

 const std::string SubQuery_CompFolder =
    " SELECT  'CompFolder' as ObjectType "
        " ,f.folder_no   as FolderNo "
        " ,f.folder_name as FolderName "
        " ,f.hierarchy   as Hierarchy "
        " ,f.parent_no   as ParentNo  "
        " ,p.folder_name as ParentFolderName "
        " FROM COMP_FOLDER f "
            " LEFT JOIN  COMP_FOLDER p on f.parent_no AND p.folder_no ";

 const std::string Query_CompFolder =
    " SELECT   CompFolder.ObjectType as ObjectType "
        " ,CompFolder.FolderNo   as FolderNo "
        " ,CompFolder.FolderName as FolderName "
        " ,CompFolder.Hierarchy  as Hierarchy "
        " ,CompFolder.ParentNo   as ParentNo "
        " ,CompFolder.ParentFolderName   as ParentFolderName "
        " FROM ("
            + SubQuery_CompFolder +
        " ) CompFolder";

 const std::string Query_CompFolderCount =
    " SELECT count(*) as TotalCount "
        " FROM ("
            + SubQuery_CompFolder +
        " ) CompFolder";

 const std::string SubQuery_CompFile =
    " SELECT  'CompFile'          as ObjectType "
        "  ,file.file_no        as FileNo "
        "  ,file.file_name      as FileName "
        "  ,file.folder_no      as FolderNo "
        "  ,folder.folder_name  as FolderName "
        "  ,file.hierarchy      as Hierarchy "
        " FROM COMP_FILE file "
        " LEFT JOIN COMP_FOLDER folder ON file.folder_no = folder.folder_no ";


 const std::string Query_CompFile =
    " SELECT   CompFile.ObjectType as ObjectType "
        " ,CompFile.FileNo     as FileNo "
        " ,CompFile.FileName   as FileName "
        " ,CompFile.FolderNo   as FolderNo "
        " ,CompFile.FolderName as FolderName "
        " ,CompFile.Hierarchy  as Hierarchy "
        " FROM ( "
            + SubQuery_CompFile +
        ") CompFile ";

 const std::string Query_CompFileCount =
    " SELECT count(*) as TotalCount "
        " FROM ( "
            + SubQuery_CompFile +
        ") CompFile ";

 const std::string Query_CompFileFolderByFolderId =
    " SELECT 'CompFolder'   as ObjectType "
        " , folder_no   as FolderNo "
        " , 0           as FileNo "
        " , folder_name as FolderName "
        " , ''          as FileName "
        " , hierarchy   as Hierarchy "
        " FROM COMP_FOLDER folder "
            " WHERE folder.parent_no = :OBJECTID "
            " UNION ALL "
                " SELECT  'CompFile'   as ObjectType "
                    " , folder_no   as FolderNo "
                    " , file_no     as FileNo "
                    " , ''          as FolderName "
                    " , file_name   as FileName "
                    " , hierarchy   as Hierarchy "
                    " FROM COMP_FILE file "
                        " WHERE file.folder_no = :OBJECTID "
                            " ORDER BY ObjectType DESC, FolderName, FileName ASC";

 const std::string Query_CompFileFolderCountByFolderId =
    " SELECT count (*) as ToTalCount "
        " FROM "
        " ("
            + Query_CompFileFolderByFolderId +
        " ) CompFileFolder";

}
}
