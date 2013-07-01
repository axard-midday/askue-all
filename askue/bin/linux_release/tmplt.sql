SELECT date_tbl.date, time_tbl.time FROM date_tbl, time_tbl                                                    
	WHERE                                                                                                   
		 (                                                                                              
	                (                                                                                       
                                NOT EXISTS ( SELECT reg_tbl.date FROM reg_tbl                                   
				                WHERE date_tbl.date = reg_tbl.date                              
				                 AND reg_tbl.cnt = 20 
				                 AND ( reg_tbl.type = 'p+' OR reg_tbl.type = 'p-' OR             
				                       reg_tbl.type = 'q+' OR reg_tbl.type = 'q-' ) )                                                  
                        OR                                                                                      
		                NOT EXISTS ( SELECT reg_tbl.time, reg_tbl.date FROM reg_tbl                     
				                WHERE time_tbl.time = reg_tbl.time                              
				                 AND reg_tbl.cnt = 20 
				                 AND ( reg_tbl.type = 'p+' OR reg_tbl.type = 'p-' OR             
				                       reg_tbl.type = 'q+' OR reg_tbl.type = 'q-' ) )                                               
			)                                                                                       
		        AND date_tbl.date = ( ( SELECT DATE ( 'now' ) ) )                                       
		        AND time_tbl.time < ( ( SELECT TIME ( 'now', '-60 minute' ) ) )                         
		)                                                                                               
		OR                                                                                              
		(                                                                                               
		        (                                                                                       
                                NOT EXISTS ( SELECT reg_tbl.date FROM reg_tbl                                   
				                WHERE date_tbl.date = reg_tbl.date                              
				                 AND reg_tbl.cnt = 20 
				                 AND ( reg_tbl.type = 'p+' OR reg_tbl.type = 'p-' OR             
				                       reg_tbl.type = 'q+' OR reg_tbl.type = 'q-' ) )                                                    
                        OR                                                                                      
		                NOT EXISTS ( SELECT reg_tbl.time, reg_tbl.date FROM reg_tbl                     
				                WHERE time_tbl.time = reg_tbl.time                              
				                 AND reg_tbl.cnt = 20 
				                 AND ( reg_tbl.type = 'p+' OR reg_tbl.type = 'p-' OR             
				                       reg_tbl.type = 'q+' OR reg_tbl.type = 'q-' ) )           
		        )                                                                                       
		        AND date_tbl.date < ( ( SELECT DATE ( 'now' ) ) )                                       
		        AND date_tbl.date > ( ( SELECT DATE ( 'now', '-31 day' ) ) )                            
		)                                                                                               
	ORDER BY date_tbl.date DESC, time_tbl.time DESC                                                         
	LIMIT 1;
