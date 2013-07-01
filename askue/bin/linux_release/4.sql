SELECT date_tbl.date, time_tbl.time FROM date_tbl, time_tbl
	WHERE  
		 (
	                (
                                NOT EXISTS ( SELECT reg_tbl.date FROM reg_tbl
				                WHERE date_tbl.date = reg_tbl.date
				                 AND reg_tbl.cnt = 234
				                 AND SUBSTR ( reg_tbl.type, 1, 1 ) = 'p' )
                        OR
		                NOT EXISTS ( SELECT reg_tbl.time, reg_tbl.date FROM reg_tbl
				                WHERE time_tbl.time = reg_tbl.time 
				                 AND reg_tbl.cnt = 234
				                 AND SUBSTR ( reg_tbl.type, 1, 1 ) = 'p' )
		        )
		        AND date_tbl.date = ( ( SELECT DATE ( 'now' ) ) )
		        AND time_tbl.time < ( ( SELECT TIME ( 'now', '-30 minute' ) ) )
		)
		OR
		(
		        (
                                NOT EXISTS ( SELECT reg_tbl.date FROM reg_tbl
				                WHERE date_tbl.date = reg_tbl.date
				                 AND reg_tbl.cnt = 234
				                 AND SUBSTR ( reg_tbl.type, 1, 1 ) = 'p' )
                        OR
		                NOT EXISTS ( SELECT reg_tbl.time, reg_tbl.date FROM reg_tbl
				                WHERE time_tbl.time = reg_tbl.time 
				                 AND reg_tbl.cnt = 234
				                 AND SUBSTR ( reg_tbl.type, 1, 1 ) = 'p' )
		        )
		        AND date_tbl.date < ( ( SELECT DATE ( 'now' ) ) )
		        AND date_tbl.date > ( ( SELECT DATE ( 'now', '-31 day' ) ) )
		)
	ORDER BY date_tbl.date DESC, time_tbl.time DESC
	LIMIT 1;
