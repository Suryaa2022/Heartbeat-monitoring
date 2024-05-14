#include "MP_MediaQueryConstant.h"

 namespace lge {
 namespace mm {

 const std::string SubQuery_SongsByPath =
    "SELECT   au.id             as SongID "
        " , au.title          as SongTitle "
        " , au.length         as Duration "
        " , f.path            as URL "
        " , f.playng          as PlayNG "
        " , au.trackno        as TrackNo "
        " , au.playcnt        as PlayCount "
        " , au.album_id       as AlbumID "
        " , al.name           as AlbumName "
        " , al.album_art_url  as CoverArt "
        " , au.artist_id      as ArtistID "
        " , ar.name           as ArtistName "
        " , au.genre_id       as GenreID "
        " , ge.name           as GenreName "
        " FROM audios au, audio_albums al, audio_artists ar, audio_genres ge, files f "
            " WHERE au.album_id = al.id "
                " AND au.artist_id = ar.id "
                " AND au.genre_id = ge.id "
                " AND f.id = au.id "
                " AND f.dtime = 0 "
                " AND f.path GLOB :PATH||'*' ";

 const std::string SubQuery_SongsByPathForSearch = SubQuery_SongsByPath
                                        + "  AND f.path NOT GLOB '/media/mtp/*' ";

 const std::string Query_SongsByPath =
        "	SELECT   Song.SongID        as SongID "
                  " ,Song.SongTitle     as SongTitle "
                  " ,Song.Duration      as Duration "
                  " ,Song.URL           as URL "
                  " ,Song.PlayNG        as PlayNG "
                  " ,Song.TrackNo       as TrackNo "
                  " ,Song.PlayCount     as PlayCount "
                  " ,Song.AlbumID       as AlbumID "
                  " ,Song.AlbumName     as AlbumName "
                  " ,Song.CoverArt      as CoverArt "
                  " ,Song.ArtistID      as ArtistID "
                  " ,Song.ArtistName    as ArtistName "
                  " ,Song.GenreID       as GenreID "
                  " ,Song.GenreName     as GenreName "
            " FROM "
            "("
                 + SubQuery_SongsByPath +
            ")Song";


 const std::string Query_SongsByPathForSearch =
        "	SELECT   Song.SongID        as SongID "
                  " ,Song.SongTitle     as SongTitle "
                  " ,Song.Duration      as Duration "
                  " ,Song.URL           as URL "
                  " ,Song.PlayNG        as PlayNG "
                  " ,Song.TrackNo       as TrackNo "
                  " ,Song.PlayCount     as PlayCount "
                  " ,Song.AlbumID       as AlbumID "
                  " ,Song.AlbumName     as AlbumName "
                  " ,Song.CoverArt      as CoverArt "
                  " ,Song.ArtistID      as ArtistID "
                  " ,Song.ArtistName    as ArtistName "
                  " ,Song.GenreID       as GenreID "
                  " ,Song.GenreName     as GenreName "
            " FROM "
            "("
                 + SubQuery_SongsByPathForSearch +
            ")Song";


 const std::string Query_SongCountByPath =
              " SELECT count(*) as TotalCount"
                " FROM "
                "("
                     + SubQuery_SongsByPath +
                ")Song";

 const std::string Query_SongCountByPathForSearch =
              " SELECT count(*) as TotalCount"
                " FROM "
                "("
                     + SubQuery_SongsByPathForSearch +
                ")Song";

//Albums
 const std::string SubQuery_AlbumsByPath =
        "         SELECT   al.id            as AlbumID "
        "                 ,al.name          as AlbumName "
        "                 ,ar.id            as ArtistID "
        "                 ,ar.name          as ArtistName "
        "                 ,al.album_art_url as CoverArt "
        "                 ,sum(au.length)   as Duration "
        "                 ,count()          as SongCount "
        "            FROM audio_albums al, audios au, files f, audio_artists ar "
        "           WHERE al.id = au.album_id "
        "             AND ar.id = al.artist_id "
        "             AND au.id = f.id "
        "             AND f.dtime = 0 "
        "             AND f.path GLOB :PATH||'*' "
        "        GROUP BY al.name "
        "               , al.id "
        "               , al.album_art_url ";

 const std::string Query_AlbumsByPath =
       " SELECT    Album.AlbumID    as AlbumID "
       "         , Album.AlbumName  as AlbumName "
       "         , Album.ArtistID   as ArtistID "
       "         , Album.ArtistName as ArtistName "
       "         , Album.CoverArt   as CoverArt"
       "         , Album.Duration   as Duration"
       "         , Album.SongCount  as SongCount"
       " FROM ( "
       +  SubQuery_AlbumsByPath +
       " ) Album ";

 const std::string Query_AlbumCountByPath =
              " SELECT count(*) as TotalCount"
                " FROM "
                "("
                     + SubQuery_AlbumsByPath +
                ")Album";

//Albums Groupping With volums.

 const std::string SubQuery_AlbumsGroupWithDevice =
        "         SELECT   al.id            as AlbumID "
                        " ,al.name          as AlbumName "
                        " ,CASE instr(f.path, '/usr_data/media') "
                         " WHEN 1 THEN 'CAR' "
                         " ELSE "
                           " CASE instr(f.path, '/media/usb') "
                           " WHEN 1 THEN 'USB' "
                           " ELSE 'ETC' END "
                         " END              as Device "
                        " ,ar.id            as ArtistID "
                        " ,ar.name          as ArtistName "
                        " ,al.album_art_url as CoverArt "
                        " ,sum(au.length)   as Duration "
                        " ,count()          as SongCount "
                  " FROM audio_albums al, audios au, files f, audio_artists ar "
                 " WHERE al.id = au.album_id "
                  "  AND ar.id = al.artist_id "
                  "  AND au.id = f.id "
                  "  AND f.dtime = 0 "
                  "  AND f.path GLOB :PATH||'*' "
             "  GROUP BY al.name, al.id "
                        " ,CASE instr(f.path, '/usr_data/media') "
                         " WHEN 1 THEN 'CAR' "
                         " ELSE "
                           " CASE instr(f.path, '/media/usb') "
                           " WHEN 1 THEN 'USB' "
                           " ELSE 'ETC' END "
                          "END"
                        " ,al.album_art_url ";


 const std::string Query_AlbumsGroupWithDevice =
       " SELECT    Album.AlbumID    as AlbumID "
       "         , Album.AlbumName  as AlbumName "
       "         , Album.Device     as Device "
       "         , Album.ArtistID   as ArtistID "
       "         , Album.ArtistName as ArtistName "
       "         , Album.CoverArt   as CoverArt"
       "         , Album.Duration   as Duration"
       "         , Album.SongCount  as SongCount"
       " FROM ( "
       +  SubQuery_AlbumsGroupWithDevice +
       " ) Album ";

 const std::string Query_AlbumCountGroupWithDevice =
              " SELECT count(*) as TotalCount"
                " FROM "
                "("
                     + SubQuery_AlbumsGroupWithDevice +
                ")Album";

//Artists
 const std::string SubQuery_ArtistsByPath =
        "  SELECT  album_stat.artist_id       as ArtistID "
                " ,ar.name                    as ArtistName "
                " ,first_album.id             as AlbumID "
                " ,first_album.name           as AlbumName "
                " ,first_album.album_art_url  as CoverArt "
                " ,album_stat.album_count     as AlbumCount "
                " ,album_stat.song_count      as SongCount "
        " FROM  "
            " (SELECT  album.artist_id as artist_id "
                    " ,count(*)        as album_count "
                    " ,sum(song_count) as song_count "
               " FROM "
                    " (SELECT  al.id         as album_id "
                    "         ,al.name       as album_name "
                    "         ,al.artist_id  as artist_id "
                    "         ,count(*)      as song_count "
                    "    FROM audios au, audio_albums al, files f "
                    "   WHERE au.album_id = al.id "
                    "     AND au.id = f.id "
                    "     AND f.dtime = 0 "
                    "     AND f.path GLOB :PATH||'*' "
                    " GROUP BY al.name, al.id)album "
            " GROUP BY album.artist_id "
            " )album_stat "
            " , "
            " ( SELECT al.artist_id  "
                    " ,MIN(al.name) as name "
                    " ,al.id "
                    " ,al.album_art_url "
              " FROM audio_albums al, audios au, files f "
               " WHERE au.album_id = al.id "
                   " AND  au.id = f.id "
                   " AND f.dtime = 0 "
            " GROUP BY al.artist_id "
            " )first_album "
            " , "
            " audio_artists ar "
        " WHERE album_stat.artist_id = ar.id "
        "  AND  album_stat.artist_id = first_album.artist_id " ;



 const std::string Query_ArtistsByPath =
          "SELECT   Artist.ArtistID     as ArtistID   "
                  ",Artist.ArtistName   as ArtistName "
                  ",Artist.AlbumID      as AlbumID "
                  ",Artist.AlbumName    as AlbumName "
                  ",Artist.CoverArt     as CoverArt   "
                  ",Artist.AlbumCount   as AlbumCount "
                  ",Artist.SongCount    as SongCount  "
          "  FROM  "
          "      ( "
          +  SubQuery_ArtistsByPath +
          " ) Artist ";

 const std::string Query_ArtistCountByPath =
              " SELECT count(*) as TotalCount"
                " FROM "
                "("
                     + SubQuery_ArtistsByPath +
                ")Artist";

 const std::string SubQuery_ArtistGroupWithDevice =
         "SELECT album_stat.artist_id       as ArtistID "
              " ,ar.name                    as ArtistName "
              " ,album_stat.device          as Device "
              " ,first_album.id             as AlbumID "
              " ,first_album.name           as AlbumName "
              " ,first_album.album_art_url  as CoverArt "
              " ,album_stat.album_count     as AlbumCount "
              " ,album_stat.song_count      as SongCount "
        " FROM "
          " (SELECT  album.artist_id as artist_id "
                  " ,album.device    as device "
                  " ,count(*)        as album_count "
                  " ,sum(song_count) as song_count "
              " FROM "
                  " (SELECT  al.id         as album_id "
                          " ,al.name       as album_name "
                          " ,al.artist_id  as artist_id "
                          " ,CASE instr(f.path, '/usr_data/media') "
                           " WHEN 1 THEN 'CAR' "
                           " ELSE "
                             " CASE instr(f.path, '/media/usb') "
                             " WHEN 1 THEN 'USB' "
                             " ELSE 'ETC' END "
                           " END           as device "
                           ",count(*)      as song_count "
                     " FROM audios au, audio_albums al, files f "
                    " WHERE au.album_id = al.id "
                      " AND au.id = f.id "
                      " AND f.dtime = 0 "
                      " AND f.path GLOB :PATH||'*' "
                 " GROUP BY al.name, al.id, "
                           " CASE instr(f.path, '/usr_data/media') "
                           " WHEN 1 THEN 'CAR' "
                           " ELSE "
                             " CASE instr(f.path, '/media/usb') "
                             " WHEN 1 THEN 'USB' "
                             " ELSE 'ETC' END "
                           " END "
                 " )album "
          " GROUP BY album.artist_id, album.device "
          " )album_stat "
          " , "
          " ( SELECT  al.artist_id "
                   " , CASE instr(f.path, '/usr_data/media') "
                           " WHEN 1 THEN 'CAR' "
                           " ELSE "
                             " CASE instr(f.path, '/media/usb') "
                             " WHEN 1 THEN 'USB' "
                             " ELSE 'ETC' END "
                           " END   as device "
                   " ,MIN(name) as name "
                   " ,al.id "
                   " ,al.album_art_url "
              " FROM audio_albums al, audios au, files f "
            " WHERE  au.album_id = al.id "
              " AND au.id = f.id "
              " AND f.dtime = 0 "
              " AND f.path GLOB :PATH||'*' "
         " GROUP BY al.artist_id, "
                 " CASE instr(f.path, '/usr_data/media') "
                          " WHEN 1 THEN 'CAR' "
                          " ELSE "
                             " CASE instr(f.path, '/media/usb') "
                             " WHEN 1 THEN 'USB' "
                             " ELSE 'ETC' END "
                          " END "
           " )first_album "
          " , "
          " audio_artists ar "
       " WHERE album_stat.artist_id = ar.id "
         " AND album_stat.artist_id = first_album.artist_id  "
         " AND album_stat.device = first_album.device ";

 const std::string Query_ArtistsGroupWithDevice =
          "SELECT   Artist.ArtistID     as ArtistID   "
                  ",Artist.ArtistName   as ArtistName "
                  ",Artist.AlbumID      as AlbumID "
                  ",Artist.AlbumName    as AlbumName "
                  ",Artist.Device       as Device "
                  ",Artist.CoverArt     as CoverArt   "
                  ",Artist.AlbumCount   as AlbumCount "
                  ",Artist.SongCount    as SongCount  "
          "  FROM  "
          "      ( "
          +  SubQuery_ArtistGroupWithDevice +
          " ) Artist ";

 const std::string Query_ArtistCountGroupWithDevice =
              " SELECT count(*) as TotalCount"
                " FROM "
                "("
                    + SubQuery_ArtistGroupWithDevice +
                ")Artist";

//Genres
//collation test
/*
 const std::string SubQuery_GenresByPath =
        " SELECT     ge.id      as GenreID "
        "           ,ge.name    as GenreName "
                  " , 0         as SongCount "
                  " , 0         as GenreDuration "
           " FROM collation_test ge"
          " WHERE  :PATH = :PATH " ;
*/
 const std::string SubQuery_GenresByPath =
        " SELECT     ge.id      as GenreID "
        "           ,ge.name    as GenreName "
                  " ,count(*)   as SongCount "
                  " ,sum(au.length) as GenreDuration "
           " FROM audio_genres ge, audios au, files f "
          " WHERE au.genre_id = ge.id "
             " AND au.id = f.id "
             " AND f.dtime = 0 "
             " AND f.path GLOB :PATH||'*' "
           " GROUP BY ge.id, ge.name ";

 const std::string Query_GenresByPath =
        "SELECT  Genre.GenreID         as GenreID "
              " ,Genre.GenreName       as GenreName "
              " ,Genre.SongCount       as SongCount "
              " ,Genre.GenreDuration   as Duration "
              "  FROM "
              "  ( "
        + SubQuery_GenresByPath +
                " ) "
                " Genre ";

 const std::string Query_GenreCountByPath =
              " SELECT count(*) TotalCount"
                " FROM "
                "("
                     + SubQuery_GenresByPath +
                ")Genre";


 const std::string SubQuery_GenresGroupWithDevice =
        " SELECT     ge.id      as GenreID "
        "           ,ge.name    as GenreName "
                  " ,CASE instr(f.path, '/usr_data/media') "
                     " WHEN 1 THEN 'CAR' "
                     " ELSE "
                       " CASE instr(f.path, '/media/usb') "
                       " WHEN 1 THEN 'USB' "
                       " ELSE 'ETC' END "
                     " END      as device "
                  " ,count(*)   as SongCount "
                  " ,sum(au.length) as GenreDuration "
           " FROM audio_genres ge, audios au, files f "
          " WHERE au.genre_id = ge.id "
             " AND au.id = f.id "
             " AND f.dtime = 0 "
             " AND f.path GLOB :PATH||'*' "
           " GROUP BY ge.id, ge.name "
                 " ,CASE instr(f.path, '/usr_data/media') "
                 " WHEN 1 THEN 'CAR' "
                 " ELSE "
                   " CASE instr(f.path, '/media/usb') "
                   " WHEN 1 THEN 'USB' "
                   " ELSE 'ETC' END "
                   " END ";

 const std::string Query_GenresGroupWithDevice =
    "SELECT  Genre.GenreID         as GenreID "
        " ,Genre.GenreName       as GenreName "
        " ,Genre.Device          as Device "
        " ,Genre.SongCount       as SongCount "
        " ,Genre.GenreDuration   as Duration "
        "  FROM "
        "  ( "
            + SubQuery_GenresGroupWithDevice +
        " ) "
        " Genre ";

 const std::string Query_GenreCountGroupWithDevice =
    " SELECT count(*) TotalCount"
        " FROM "
        "("
            + SubQuery_GenresGroupWithDevice +
        ")Genre";


 const std::string Query_SongCountStatByDevice =
    "SELECT   stat.Device as Device "
        " , COUNT(*)  as Count "
        " FROM "
        " ( "
            " SELECT   CASE instr(f.path, '/usr_data/media') "
                " WHEN 1 THEN 'CAR' "
                " ELSE "
                "   CASE instr(f.path, '/media/usb') "
                "  WHEN 1 THEN 'USB' "
                "  ELSE 'ETC' END "
                " END  as Device "
                " FROM audios a, files f "
                    " WHERE a.id = f.id "
                    " AND  f.dtime = 0 "
        " ) stat "
        " GROUP BY stat.Device ";

 const std::string Query_RemoteVideosByPath =
    "SELECT   Remote.VideoID       as VideoID "
           " ,Remote.VideoTitle    as VideoTitle "
           " ,Remote.Duration      as Duration "
           " ,Remote.URL           as URL "
           " ,Remote.PlayNG        as PlayNG "
        " FROM "
        "("
            "SELECT   vi.id             as VideoID "
                  " , vi.title          as VideoTitle "
                  " , vi.length         as Duration "
                  " , vi.container      as Container "
                  " , f.path            as URL "
                  " , f.playng          as PlayNG "
                  " , vv.video_id       as VideoFileID "
                  " , vv.width          as Width "
                  " , vv.height         as Height "
            " FROM videos vi, videos_videos vv, files f "
                " WHERE vv.video_id = vi.id "
                  " AND f.id = vi.id "
                  " AND f.dtime = 0 "
                  " AND f.path GLOB :PATH||'*' "
        ")Remote";
  } // mm
} // lge
